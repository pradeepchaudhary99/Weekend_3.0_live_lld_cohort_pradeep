"""
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
"""

import queue
import threading
import time
from enum import Enum, auto
from itertools import count
from typing import Callable, Dict, List, Optional
from uuid import uuid4

TaskAction = Callable[[], None]


class TaskStatus(Enum):
    PENDING = auto()
    RUNNING = auto()
    COMPLETED = auto()
    FAILED = auto()
    CANCELLED = auto()


class RecurrenceType(Enum):
    ONE_TIME = auto()
    FIXED_DELAY = auto()


class ScheduledTask:
    def __init__(self, name: str, priority: int, recurrence_type: RecurrenceType):
        self.id = str(uuid4())
        self.name = name
        self.priority = priority
        self.recurrence_type = recurrence_type
        self.status = TaskStatus.PENDING
        self.lock = threading.Lock()


_sequence_counter = count()


class _PrioritizedItem:
    """Wraps a runnable with a (priority, sequence) sort key so a plain
    queue.PriorityQueue (a min-heap) runs higher-priority items first,
    breaking ties in submission order."""

    def __init__(self, priority: int, runnable: Callable[[], None]):
        self.sort_key = (-priority, next(_sequence_counter))
        self.runnable = runnable

    def __lt__(self, other: "_PrioritizedItem") -> bool:
        return self.sort_key < other.sort_key


class TaskScheduler:
    def __init__(self):
        self._tasks: Dict[str, ScheduledTask] = {}
        self._timers: Dict[str, threading.Timer] = {}
        self._done_events: Dict[str, threading.Event] = {}
        self._execution_log: List[str] = []
        self._log_lock = threading.Lock()

        self._work_queue: "queue.PriorityQueue[_PrioritizedItem]" = queue.PriorityQueue()
        self._worker = threading.Thread(target=self._run_worker, daemon=True)
        self._worker.start()

    def _run_worker(self) -> None:
        while True:
            item = self._work_queue.get()
            if item is None:  # shutdown sentinel
                break
            item.runnable()

    def submit(self, name: str, action: TaskAction, priority: int) -> str:
        task = ScheduledTask(name, priority, RecurrenceType.ONE_TIME)
        self._tasks[task.id] = task
        done_event = threading.Event()
        self._done_events[task.id] = done_event
        runnable = self._wrap(task, action, on_done=done_event.set)
        self._work_queue.put(_PrioritizedItem(priority, runnable))
        return task.id

    def schedule(self, name: str, action: TaskAction, priority: int,
                 initial_delay_ms: int, period_ms: int, recurrence_type: RecurrenceType) -> str:
        task = ScheduledTask(name, priority, recurrence_type)
        self._tasks[task.id] = task
        runnable = self._wrap(task, action)

        def fire() -> None:
            with task.lock:
                cancelled = task.status == TaskStatus.CANCELLED
            if cancelled:
                return
            runnable()
            with task.lock:
                still_active = task.status != TaskStatus.CANCELLED
            if recurrence_type == RecurrenceType.FIXED_DELAY and still_active:
                timer = threading.Timer(period_ms / 1000, fire)
                timer.daemon = True
                self._timers[task.id] = timer
                timer.start()

        timer = threading.Timer(initial_delay_ms / 1000, fire)
        timer.daemon = True
        self._timers[task.id] = timer
        timer.start()
        return task.id

    def cancel(self, task_id: str) -> bool:
        task = self._tasks.get(task_id)
        if task is None:
            return False
        with task.lock:
            if task.status != TaskStatus.PENDING:
                return False  # already running/finished, too late to cancel
            task.status = TaskStatus.CANCELLED
        timer = self._timers.get(task_id)
        if timer is not None:
            timer.cancel()
        return True

    def get_status(self, task_id: str) -> Optional[TaskStatus]:
        task = self._tasks.get(task_id)
        return task.status if task else None

    def await_completion(self, task_id: str) -> None:
        event = self._done_events.get(task_id)
        if event is not None:
            event.wait()

    def get_execution_log(self) -> List[str]:
        with self._log_lock:
            return list(self._execution_log)

    def shutdown(self) -> None:
        for timer in self._timers.values():
            timer.cancel()
        self._work_queue.put(None)
        self._worker.join(timeout=2)

    def _wrap(self, task: ScheduledTask, action: TaskAction,
              on_done: Optional[Callable[[], None]] = None) -> Callable[[], None]:
        def runnable() -> None:
            with task.lock:
                if task.status == TaskStatus.CANCELLED:
                    if on_done:
                        on_done()
                    return
                task.status = TaskStatus.RUNNING
            try:
                action()
                with task.lock:
                    if task.status != TaskStatus.CANCELLED:
                        # A recurring task goes back to PENDING, ready for its next tick.
                        task.status = (
                            TaskStatus.PENDING
                            if task.recurrence_type == RecurrenceType.FIXED_DELAY
                            else TaskStatus.COMPLETED
                        )
                with self._log_lock:
                    self._execution_log.append(f"Executed {task.name} (priority={task.priority})")
            except Exception as error:
                task.status = TaskStatus.FAILED
                with self._log_lock:
                    self._execution_log.append(f"Failed {task.name}: {error}")
            finally:
                if on_done:
                    on_done()

        return runnable


def main() -> None:
    scheduler = TaskScheduler()

    # Barrier task: highest possible priority guarantees it is dequeued before
    # the three real submissions below, no matter whether the worker thread
    # happens to grab it alone or finds all four already queued together. It
    # then sleeps briefly so the other submissions are guaranteed to have
    # landed in the queue before it finishes.
    barrier_id = scheduler.submit("barrier", lambda: time.sleep(0.05), 10**9)

    ids = [
        scheduler.submit("low-priority-report", lambda: None, 1),
        scheduler.submit("high-priority-alert", lambda: None, 10),
        scheduler.submit("medium-priority-sync", lambda: None, 5),
    ]

    scheduler.await_completion(barrier_id)
    for task_id in ids:
        scheduler.await_completion(task_id)

    print("Execution order (higher priority runs first):")
    for line in scheduler.get_execution_log():
        print(line)

    # Cancel a pending, delayed one-time task before it ever executes.
    delayed_id = scheduler.schedule("delayed-cleanup", lambda: None, 1, 300, 0, RecurrenceType.ONE_TIME)
    cancelled = scheduler.cancel(delayed_id)
    time.sleep(0.4)
    print(f"\nDelayed task cancelled={cancelled}, status={scheduler.get_status(delayed_id).name}")

    # Recurring task: let it tick a few times, then cancel and confirm it stops.
    tick_count = [0]

    def heartbeat() -> None:
        tick_count[0] += 1

    recurring_id = scheduler.schedule("heartbeat", heartbeat, 1, 0, 100, RecurrenceType.FIXED_DELAY)
    time.sleep(0.35)
    scheduler.cancel(recurring_id)
    count_at_cancel = tick_count[0]
    time.sleep(0.3)
    print(f"\nHeartbeat ticks at cancel time={count_at_cancel}, ticks after waiting longer={tick_count[0]}")

    scheduler.shutdown()


if __name__ == "__main__":
    main()
