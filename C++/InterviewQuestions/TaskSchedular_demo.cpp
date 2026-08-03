/*
Task Scheduler Demo
------------------------------

Functional:
    1. Submit a task for immediate execution
    2. Schedule a task to run on repeat every <period>
    3. Support task priority so higher-priority tasks run first
    4. Cancel a pending task before it executes
    5. Every task has multiple states: PENDING, RUNNING, COMPLETED, FAILED, CANCELLED
    6. Execution is asynchronous: submission returns immediately, work happens
       on a worker thread

Non-Functional Requirements:
    1. No busy-polling
    2. No double execution
    3. Extensibility: multiple new types of task support

Entities:
    TaskAction
    ScheduledTask
    TaskStatus
    RecurrenceType --> ONE_TIME, FIXED_DELAY
*/

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using TaskAction = std::function<void()>;

enum class TaskStatus { PENDING, RUNNING, COMPLETED, FAILED, CANCELLED };

enum class RecurrenceType { ONE_TIME, FIXED_DELAY };

std::string statusName(TaskStatus status) {
    switch (status) {
        case TaskStatus::PENDING: return "PENDING";
        case TaskStatus::RUNNING: return "RUNNING";
        case TaskStatus::COMPLETED: return "COMPLETED";
        case TaskStatus::FAILED: return "FAILED";
        case TaskStatus::CANCELLED: return "CANCELLED";
    }
    return "UNKNOWN";
}

struct ScheduledTask {
    std::string id;
    std::string name;
    long priority;
    RecurrenceType recurrenceType;
    std::atomic<TaskStatus> status{TaskStatus::PENDING};
    std::mutex statusMutex;

    ScheduledTask(std::string id, std::string name, long priority, RecurrenceType recurrenceType)
        : id(std::move(id)), name(std::move(name)), priority(priority), recurrenceType(recurrenceType) {}
};

namespace {
std::atomic<long> idCounter{0};
std::atomic<long> sequenceCounter{0};

std::string generateId(const std::string& prefix) {
    return prefix + std::to_string(idCounter.fetch_add(1));
}
}  // namespace

// Wraps a runnable with a (priority, sequence) sort key so std::priority_queue
// (a max-heap by default) runs higher-priority items first, breaking ties in
// submission order.
struct PrioritizedItem {
    long priority;
    long sequence;
    std::function<void()> runnable;

    bool operator<(const PrioritizedItem& other) const {
        if (priority != other.priority) return priority < other.priority;  // higher priority first
        return sequence > other.sequence;  // lower sequence (submitted earlier) first
    }
};

// A single-worker priority queue: no busy-polling (blocks on a condition
// variable), and only one thread ever pulls work so no task runs twice.
class TaskScheduler {
public:
    TaskScheduler() : worker_(&TaskScheduler::runWorker, this) {}

    std::string submit(const std::string& name, TaskAction action, long priority) {
        auto task = std::make_shared<ScheduledTask>(generateId("task-"), name, priority, RecurrenceType::ONE_TIME);
        auto doneFlag = std::make_shared<DoneFlag>();
        {
            std::lock_guard<std::mutex> lock(registryMutex_);
            tasks_[task->id] = task;
            doneFlags_[task->id] = doneFlag;
        }
        auto runnable = wrap(task, std::move(action), doneFlag);
        enqueue(priority, std::move(runnable));
        return task->id;
    }

    std::string schedule(const std::string& name, TaskAction action, long priority, long initialDelayMs,
                          long periodMs, RecurrenceType type) {
        auto task = std::make_shared<ScheduledTask>(generateId("task-"), name, priority, type);
        {
            std::lock_guard<std::mutex> lock(registryMutex_);
            tasks_[task->id] = task;
        }
        auto runnable = wrap(task, action, nullptr);

        auto timerThread = std::make_shared<TimerThread>();
        {
            std::lock_guard<std::mutex> lock(registryMutex_);
            timers_[task->id] = timerThread;
        }

        timerThread->thread = std::thread([task, runnable, initialDelayMs, periodMs, type, timerThread]() {
            std::unique_lock<std::mutex> lock(timerThread->mutex);
            if (timerThread->cv.wait_for(lock, std::chrono::milliseconds(initialDelayMs),
                                          [&] { return timerThread->cancelled; })) {
                return;
            }
            while (true) {
                lock.unlock();
                runnable();
                lock.lock();
                if (timerThread->cancelled || type == RecurrenceType::ONE_TIME) {
                    return;
                }
                if (timerThread->cv.wait_for(lock, std::chrono::milliseconds(periodMs),
                                              [&] { return timerThread->cancelled; })) {
                    return;
                }
            }
        });
        return task->id;
    }

    bool cancel(const std::string& taskId) {
        std::shared_ptr<ScheduledTask> task;
        {
            std::lock_guard<std::mutex> lock(registryMutex_);
            auto it = tasks_.find(taskId);
            if (it == tasks_.end()) return false;
            task = it->second;
        }
        {
            std::lock_guard<std::mutex> lock(task->statusMutex);
            if (task->status.load() != TaskStatus::PENDING) return false;
            task->status.store(TaskStatus::CANCELLED);
        }
        std::shared_ptr<TimerThread> timerThread;
        {
            std::lock_guard<std::mutex> lock(registryMutex_);
            auto it = timers_.find(taskId);
            if (it != timers_.end()) timerThread = it->second;
        }
        if (timerThread) {
            {
                std::lock_guard<std::mutex> lock(timerThread->mutex);
                timerThread->cancelled = true;
            }
            timerThread->cv.notify_all();
        }
        return true;
    }

    TaskStatus getStatus(const std::string& taskId) {
        std::lock_guard<std::mutex> lock(registryMutex_);
        auto it = tasks_.find(taskId);
        return it == tasks_.end() ? TaskStatus::CANCELLED : it->second->status.load();
    }

    void awaitCompletion(const std::string& taskId) {
        std::shared_ptr<DoneFlag> doneFlag;
        {
            std::lock_guard<std::mutex> lock(registryMutex_);
            auto it = doneFlags_.find(taskId);
            if (it == doneFlags_.end()) return;
            doneFlag = it->second;
        }
        std::unique_lock<std::mutex> lock(doneFlag->mutex);
        doneFlag->cv.wait(lock, [&] { return doneFlag->done; });
    }

    std::vector<std::string> getExecutionLog() {
        std::lock_guard<std::mutex> lock(logMutex_);
        return executionLog_;
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(registryMutex_);
            for (auto& [id, timerThread] : timers_) {
                {
                    std::lock_guard<std::mutex> tlock(timerThread->mutex);
                    timerThread->cancelled = true;
                }
                timerThread->cv.notify_all();
            }
        }
        for (auto& [id, timerThread] : timers_) {
            if (timerThread->thread.joinable()) timerThread->thread.join();
        }
        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            shuttingDown_ = true;
        }
        queueCv_.notify_all();
        if (worker_.joinable()) worker_.join();
    }

    ~TaskScheduler() { shutdown(); }

private:
    struct DoneFlag {
        std::mutex mutex;
        std::condition_variable cv;
        bool done = false;
    };

    struct TimerThread {
        std::thread thread;
        std::mutex mutex;
        std::condition_variable cv;
        bool cancelled = false;
    };

    void enqueue(long priority, std::function<void()> runnable) {
        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            queue_.push(PrioritizedItem{priority, sequenceCounter.fetch_add(1), std::move(runnable)});
        }
        queueCv_.notify_one();
    }

    void runWorker() {
        while (true) {
            std::function<void()> runnable;
            {
                std::unique_lock<std::mutex> lock(queueMutex_);
                queueCv_.wait(lock, [&] { return shuttingDown_ || !queue_.empty(); });
                if (shuttingDown_ && queue_.empty()) return;
                runnable = queue_.top().runnable;
                queue_.pop();
            }
            runnable();
        }
    }

    std::function<void()> wrap(std::shared_ptr<ScheduledTask> task, TaskAction action,
                                std::shared_ptr<DoneFlag> doneFlag) {
        return [this, task, action, doneFlag]() {
            {
                std::lock_guard<std::mutex> lock(task->statusMutex);
                if (task->status.load() == TaskStatus::CANCELLED) {
                    signalDone(doneFlag);
                    return;
                }
                task->status.store(TaskStatus::RUNNING);
            }
            try {
                action();
                {
                    std::lock_guard<std::mutex> lock(task->statusMutex);
                    if (task->status.load() != TaskStatus::CANCELLED) {
                        // A recurring task goes back to PENDING, ready for its next tick.
                        task->status.store(task->recurrenceType == RecurrenceType::FIXED_DELAY
                                                ? TaskStatus::PENDING
                                                : TaskStatus::COMPLETED);
                    }
                }
                std::lock_guard<std::mutex> lock(logMutex_);
                executionLog_.push_back("Executed " + task->name + " (priority=" + std::to_string(task->priority) + ")");
            } catch (const std::exception& e) {
                task->status.store(TaskStatus::FAILED);
                std::lock_guard<std::mutex> lock(logMutex_);
                executionLog_.push_back("Failed " + task->name + ": " + e.what());
            }
            signalDone(doneFlag);
        };
    }

    static void signalDone(const std::shared_ptr<DoneFlag>& doneFlag) {
        if (!doneFlag) return;
        {
            std::lock_guard<std::mutex> lock(doneFlag->mutex);
            doneFlag->done = true;
        }
        doneFlag->cv.notify_all();
    }

    std::unordered_map<std::string, std::shared_ptr<ScheduledTask>> tasks_;
    std::unordered_map<std::string, std::shared_ptr<DoneFlag>> doneFlags_;
    std::unordered_map<std::string, std::shared_ptr<TimerThread>> timers_;
    std::mutex registryMutex_;

    std::vector<std::string> executionLog_;
    std::mutex logMutex_;

    std::priority_queue<PrioritizedItem> queue_;
    std::mutex queueMutex_;
    std::condition_variable queueCv_;
    bool shuttingDown_ = false;

    std::thread worker_;
};

int main() {
    TaskScheduler scheduler;

    // Barrier task: highest possible priority guarantees it is dequeued
    // before the three real submissions below, no matter whether the worker
    // thread happens to grab it alone or finds all four already queued
    // together. It then sleeps briefly so the other submissions are
    // guaranteed to have landed in the queue before it finishes.
    std::string barrierId = scheduler.submit(
        "barrier", [] { std::this_thread::sleep_for(std::chrono::milliseconds(50)); },
        std::numeric_limits<long>::max());

    std::vector<std::string> ids;
    ids.push_back(scheduler.submit("low-priority-report", [] {}, 1));
    ids.push_back(scheduler.submit("high-priority-alert", [] {}, 10));
    ids.push_back(scheduler.submit("medium-priority-sync", [] {}, 5));

    scheduler.awaitCompletion(barrierId);
    for (auto& id : ids) scheduler.awaitCompletion(id);

    std::cout << "Execution order (higher priority runs first):" << std::endl;
    for (auto& line : scheduler.getExecutionLog()) std::cout << line << std::endl;

    // Cancel a pending, delayed one-time task before it ever executes.
    std::string delayedId = scheduler.schedule("delayed-cleanup", [] {}, 1, 300, 0, RecurrenceType::ONE_TIME);
    bool cancelled = scheduler.cancel(delayedId);
    std::this_thread::sleep_for(std::chrono::milliseconds(400));
    std::cout << "\nDelayed task cancelled=" << (cancelled ? "true" : "false")
              << ", status=" << statusName(scheduler.getStatus(delayedId)) << std::endl;

    // Recurring task: let it tick a few times, then cancel and confirm it stops.
    std::atomic<int> tickCount{0};
    std::string recurringId = scheduler.schedule(
        "heartbeat", [&tickCount] { tickCount.fetch_add(1); }, 1, 0, 100, RecurrenceType::FIXED_DELAY);
    std::this_thread::sleep_for(std::chrono::milliseconds(350));
    scheduler.cancel(recurringId);
    int countAtCancel = tickCount.load();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    std::cout << "\nHeartbeat ticks at cancel time=" << countAtCancel
              << ", ticks after waiting longer=" << tickCount.load() << std::endl;

    scheduler.shutdown();
    return 0;
}
