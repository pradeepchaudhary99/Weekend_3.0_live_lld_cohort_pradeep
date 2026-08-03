/*
Functional:
    1. Submit a task for immediate execution
    2. Schedule a tak to run on repeat every
    3. support task priority so higher-priority
    4. cancel a pedning task before it executes
    5. Every Task will have multiple states : PENDING, RUNNING, COMPLETED, FAILED, CANCELLED...
    6. Execution must be asynchronous, submission happens immediately, work happens on a worker pool.

Non Functional Requirements:
    1. No busy-polling
    2. No double execution
    3. Exetensibility: multiple new types of task support
*/

// Core Entities:
/*
TaskAction
ScheduledTask
TaskStatus

RecuurenceType ---> ONE_TIME, FIXED_DELAY
*/

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.PriorityBlockingQueue;
import java.util.concurrent.RunnableFuture;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicLong;

enum TaskStatus {
    PENDING, RUNNING, COMPLETED, FAILED, CANCELLED
}

enum RecurrenceType {
    ONE_TIME, FIXED_DELAY
}

interface TaskAction {
    void run() throws Exception;
}

interface PrioritizedRunnable extends Runnable {
    int getPriority();
}

// FutureTask instances are what actually sit in the executor's work queue, so
// priority has to travel on the future itself, not the Runnable that was submitted.
class PriorityFuture<T> extends java.util.concurrent.FutureTask<T> implements Comparable<PriorityFuture<T>> {
    private final int priority;
    private final long sequence;

    PriorityFuture(Runnable runnable, T result, int priority, long sequence) {
        super(runnable, result);
        this.priority = priority;
        this.sequence = sequence;
    }

    @Override
    public int compareTo(PriorityFuture<T> other) {
        int cmp = Integer.compare(other.priority, this.priority); // higher priority first
        return cmp != 0 ? cmp : Long.compare(this.sequence, other.sequence); // FIFO tie-break
    }
}

// A single-worker thread pool whose queue is priority-ordered instead of FIFO.
class PriorityThreadPoolExecutor extends ThreadPoolExecutor {
    private final AtomicLong sequenceGenerator = new AtomicLong();

    PriorityThreadPoolExecutor(int poolSize) {
        super(poolSize, poolSize, 0L, TimeUnit.MILLISECONDS, new PriorityBlockingQueue<>());
    }

    @Override
    protected <T> RunnableFuture<T> newTaskFor(Runnable runnable, T value) {
        int priority = runnable instanceof PrioritizedRunnable ? ((PrioritizedRunnable) runnable).getPriority() : 0;
        return new PriorityFuture<>(runnable, value, priority, sequenceGenerator.incrementAndGet());
    }
}

class ScheduledTask {
    final String id = UUID.randomUUID().toString();
    final String name;
    final int priority;
    final RecurrenceType recurrenceType;
    volatile TaskStatus status = TaskStatus.PENDING;

    ScheduledTask(String name, int priority, RecurrenceType recurrenceType) {
        this.name = name;
        this.priority = priority;
        this.recurrenceType = recurrenceType;
    }
}

class TaskScheduler {
    private final Map<String, ScheduledTask> tasks = new ConcurrentHashMap<>();
    private final Map<String, Future<?>> immediateFutures = new ConcurrentHashMap<>();
    private final Map<String, ScheduledFuture<?>> scheduledFutures = new ConcurrentHashMap<>();
    private final List<String> executionLog = new ArrayList<>();

    private final PriorityThreadPoolExecutor workerPool = new PriorityThreadPoolExecutor(1);
    private final ScheduledExecutorService timer = Executors.newSingleThreadScheduledExecutor();

    String submit(String name, TaskAction action, int priority) {
        ScheduledTask task = new ScheduledTask(name, priority, RecurrenceType.ONE_TIME);
        tasks.put(task.id, task);
        Runnable prioritized = asPrioritized(wrap(task, action), priority);
        immediateFutures.put(task.id, workerPool.submit(prioritized));
        return task.id;
    }

    String schedule(String name, TaskAction action, int priority, long initialDelayMs, long periodMs, RecurrenceType type) {
        ScheduledTask task = new ScheduledTask(name, priority, type);
        tasks.put(task.id, task);
        Runnable runnable = wrap(task, action);
        ScheduledFuture<?> future = type == RecurrenceType.FIXED_DELAY
                ? timer.scheduleWithFixedDelay(runnable, initialDelayMs, periodMs, TimeUnit.MILLISECONDS)
                : timer.schedule(runnable, initialDelayMs, TimeUnit.MILLISECONDS);
        scheduledFutures.put(task.id, future);
        return task.id;
    }

    boolean cancel(String taskId) {
        ScheduledTask task = tasks.get(taskId);
        if (task == null) {
            return false;
        }
        synchronized (task) {
            if (task.status != TaskStatus.PENDING) {
                return false; // already running/finished, too late to cancel
            }
            task.status = TaskStatus.CANCELLED;
        }
        Future<?> immediate = immediateFutures.get(taskId);
        if (immediate != null) {
            immediate.cancel(false);
        }
        ScheduledFuture<?> scheduled = scheduledFutures.get(taskId);
        if (scheduled != null) {
            scheduled.cancel(false);
        }
        return true;
    }

    TaskStatus getStatus(String taskId) {
        ScheduledTask task = tasks.get(taskId);
        return task == null ? null : task.status;
    }

    void awaitCompletion(String taskId) {
        Future<?> future = immediateFutures.get(taskId);
        if (future == null) {
            return;
        }
        try {
            future.get();
        } catch (Exception ignored) {
            // status already recorded by the wrapped task
        }
    }

    List<String> getExecutionLog() {
        synchronized (executionLog) {
            return new ArrayList<>(executionLog);
        }
    }

    void shutdown() throws InterruptedException {
        timer.shutdown();
        workerPool.shutdown();
        timer.awaitTermination(2, TimeUnit.SECONDS);
        workerPool.awaitTermination(2, TimeUnit.SECONDS);
    }

    private Runnable asPrioritized(Runnable delegate, int priority) {
        return new PrioritizedRunnable() {
            @Override
            public void run() {
                delegate.run();
            }

            @Override
            public int getPriority() {
                return priority;
            }
        };
    }

    private Runnable wrap(ScheduledTask task, TaskAction action) {
        return () -> {
            synchronized (task) {
                if (task.status == TaskStatus.CANCELLED) {
                    return;
                }
                task.status = TaskStatus.RUNNING;
            }
            try {
                action.run();
                synchronized (task) {
                    if (task.status != TaskStatus.CANCELLED) {
                        // A recurring task goes back to PENDING, ready for its next tick.
                        task.status = task.recurrenceType == RecurrenceType.FIXED_DELAY
                                ? TaskStatus.PENDING
                                : TaskStatus.COMPLETED;
                    }
                }
                synchronized (executionLog) {
                    executionLog.add("Executed " + task.name + " (priority=" + task.priority + ")");
                }
            } catch (Exception e) {
                task.status = TaskStatus.FAILED;
                synchronized (executionLog) {
                    executionLog.add("Failed " + task.name + ": " + e.getMessage());
                }
            }
        };
    }
}

public class TaskSchedular_demo {
    public static void main(String[] args) throws InterruptedException {
        TaskScheduler scheduler = new TaskScheduler();

        // Barrier task: highest possible priority guarantees it is dequeued
        // before the three real submissions below, no matter whether the
        // worker thread happens to grab it alone or finds all four already
        // queued together. It then sleeps briefly so the other submissions
        // are guaranteed to have landed in the queue before it finishes.
        String barrierId = scheduler.submit("barrier", () -> Thread.sleep(50), Integer.MAX_VALUE);

        List<String> ids = new ArrayList<>();
        ids.add(scheduler.submit("low-priority-report", () -> {}, 1));
        ids.add(scheduler.submit("high-priority-alert", () -> {}, 10));
        ids.add(scheduler.submit("medium-priority-sync", () -> {}, 5));

        scheduler.awaitCompletion(barrierId);
        for (String id : ids) {
            scheduler.awaitCompletion(id);
        }

        System.out.println("Execution order (higher priority runs first):");
        scheduler.getExecutionLog().forEach(System.out::println);

        // Cancel a pending, delayed one-time task before it ever executes.
        String delayedId = scheduler.schedule("delayed-cleanup", () -> {}, 1, 300, 0, RecurrenceType.ONE_TIME);
        boolean cancelled = scheduler.cancel(delayedId);
        Thread.sleep(400);
        System.out.println("\nDelayed task cancelled=" + cancelled + ", status=" + scheduler.getStatus(delayedId));

        // Recurring task: let it tick a few times, then cancel and confirm it stops.
        int[] tickCount = {0};
        String recurringId = scheduler.schedule("heartbeat", () -> tickCount[0]++, 1, 0, 100, RecurrenceType.FIXED_DELAY);
        Thread.sleep(350);
        scheduler.cancel(recurringId);
        int countAtCancel = tickCount[0];
        Thread.sleep(300);
        System.out.println("\nHeartbeat ticks at cancel time=" + countAtCancel + ", ticks after waiting longer=" + tickCount[0]);

        scheduler.shutdown();
    }
}
