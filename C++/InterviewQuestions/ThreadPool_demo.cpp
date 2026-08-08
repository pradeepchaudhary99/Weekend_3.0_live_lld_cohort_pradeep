// ThreadPool
//     ===> Workers
//     ===> BlockingQueue
//     ===> Task

// Producer ----> adding task to the queue
// Consumer ----> Picking the task from the queue and processing it..

#include <condition_variable>
#include <deque>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

struct Task {
    virtual void execute() = 0;
    virtual ~Task() = default;
};

class RunnableTask : public Task {
public:
    explicit RunnableTask(std::function<void()> task) : task_(std::move(task)) {}

    void execute() override {
        task_();
    }

private:
    std::function<void()> task_;
};

class TaskQueue {
public:
    explicit TaskQueue(size_t capacity) : capacity_(capacity) {}

    void enqueue(std::shared_ptr<Task> task) {
        std::unique_lock<std::mutex> lock(mutex_);
        notFull_.wait(lock, [this] { return queue_.size() < capacity_; });
        queue_.push_back(std::move(task));
        notEmpty_.notify_all();  // producer asking consumer to wake up
    }

    std::shared_ptr<Task> dequeue() {
        std::unique_lock<std::mutex> lock(mutex_);
        notEmpty_.wait(lock, [this] { return !queue_.empty(); });
        std::shared_ptr<Task> task = queue_.front();
        queue_.pop_front();
        notFull_.notify_all();  // consumer asking producer to wake up
        if (queue_.empty()) {
            isEmpty_.notify_all();
        }
        return task;
    }

    void waitUntilEmpty() {
        std::unique_lock<std::mutex> lock(mutex_);
        isEmpty_.wait(lock, [this] { return queue_.empty(); });
    }

private:
    std::deque<std::shared_ptr<Task>> queue_;
    size_t capacity_;
    std::mutex mutex_;
    std::condition_variable notFull_;
    std::condition_variable notEmpty_;
    std::condition_variable isEmpty_;
};

class Worker {
public:
    explicit Worker(TaskQueue& taskQueue) : taskQueue_(taskQueue), running_(true) {}

    void shutdown() {
        running_ = false;
    }

    void run() {
        while (running_) {
            std::shared_ptr<Task> task = taskQueue_.dequeue();
            if (task != nullptr) {
                task->execute();
            }
        }
    }

private:
    TaskQueue& taskQueue_;
    bool running_;
};

class ThreadPool {
public:
    explicit ThreadPool(int numberOfThreads, size_t capacity = 10)
        : taskQueue_(capacity), numberOfThreads_(numberOfThreads), shutdown_(false) {
        createWorkers();
    }

    void submit(std::shared_ptr<Task> task) {
        taskQueue_.enqueue(std::move(task));
    }

    void shutdown() {
        shutdown_ = true;
        taskQueue_.waitUntilEmpty();  // let already-submitted tasks finish
        for (auto& worker : workers_) {
            worker->shutdown();
        }
        // wake up any workers blocked waiting for a task so they can exit
        for (size_t i = 0; i < workers_.size(); i++) {
            taskQueue_.enqueue(std::make_shared<RunnableTask>([] {}));
        }
        for (auto& thread : threads_) {
            thread.join();
        }
    }

    ~ThreadPool() {
        if (!shutdown_) {
            shutdown();
        }
    }

private:
    void createWorkers() {
        for (int i = 0; i < numberOfThreads_; i++) {
            auto worker = std::make_unique<Worker>(taskQueue_);
            Worker* workerPtr = worker.get();
            threads_.emplace_back([workerPtr] { workerPtr->run(); });
            workers_.push_back(std::move(worker));
        }
    }

    TaskQueue taskQueue_;
    int numberOfThreads_;
    bool shutdown_;
    std::vector<std::unique_ptr<Worker>> workers_;
    std::vector<std::thread> threads_;
};

int main() {
    ThreadPool pool(3);

    for (int i = 0; i < 5; i++) {
        pool.submit(std::make_shared<RunnableTask>([i] {
            std::cout << "Executing task " << i << std::endl;
        }));
    }

    pool.shutdown();

    return 0;
}
