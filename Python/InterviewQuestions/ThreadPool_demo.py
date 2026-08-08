
# ThreadPool
#     ===> Workers
#     ===> BlockingQueue
#     ===> Task

# Producer ----> adding task to the queue
# Consumer ----> Picking the task from the queue and processing it..

import threading
from abc import ABC, abstractmethod
from collections import deque
from typing import Callable, Deque, List, Optional


class Task(ABC):
    @abstractmethod
    def execute(self) -> None:
        ...


class RunnableTask(Task):
    def __init__(self, task: Callable[[], None]):
        self.task = task

    def execute(self) -> None:
        self.task()


class TaskQueue:
    def __init__(self, capacity: int):
        self.queue: Deque[Task] = deque()
        self.capacity = capacity
        self.lock = threading.Lock()
        self.not_full = threading.Condition(self.lock)
        self.not_empty = threading.Condition(self.lock)
        self.is_empty = threading.Condition(self.lock)

    def enqueue(self, task: Task) -> None:
        with self.not_full:
            while len(self.queue) == self.capacity:
                self.not_full.wait()
            self.queue.append(task)
            self.not_empty.notify_all()  # producer asking consumer to wake up

    def dequeue(self) -> Optional[Task]:
        with self.not_empty:
            while not self.queue:
                self.not_empty.wait()
            task = self.queue.popleft()
            self.not_full.notify_all()  # consumer asking producer to wake up
            if not self.queue:
                self.is_empty.notify_all()
            return task

    def wait_until_empty(self) -> None:
        with self.is_empty:
            while self.queue:
                self.is_empty.wait()


class Worker:
    def __init__(self, task_queue: TaskQueue):
        self.task_queue = task_queue
        self.running = True

    def shutdown(self) -> None:
        self.running = False

    def run(self) -> None:
        while self.running:
            task = self.task_queue.dequeue()
            if task is not None:
                task.execute()


class ThreadPool:
    def __init__(self, number_of_threads: int, capacity: int = 10):
        self.task_queue = TaskQueue(capacity)
        self.number_of_threads = number_of_threads
        self.workers: List[Worker] = []
        self.threads: List[threading.Thread] = []
        self.is_shutdown = False
        self._create_workers()

    def _create_workers(self) -> None:
        for _ in range(self.number_of_threads):
            worker = Worker(self.task_queue)
            thread = threading.Thread(target=worker.run, daemon=True)
            self.workers.append(worker)
            self.threads.append(thread)
            thread.start()

    def submit(self, task: Task) -> None:
        self.task_queue.enqueue(task)

    def shutdown(self) -> None:
        self.is_shutdown = True
        self.task_queue.wait_until_empty()  # let already-submitted tasks finish
        for worker in self.workers:
            worker.shutdown()
        # wake up any workers blocked waiting for a task so they can exit
        for _ in self.workers:
            self.task_queue.enqueue(RunnableTask(lambda: None))
        for thread in self.threads:
            thread.join()


def main() -> None:
    pool = ThreadPool(number_of_threads=3)

    for i in range(5):
        pool.submit(RunnableTask(lambda i=i: print(f"Executing task {i}")))

    pool.shutdown()


if __name__ == "__main__":
    main()
