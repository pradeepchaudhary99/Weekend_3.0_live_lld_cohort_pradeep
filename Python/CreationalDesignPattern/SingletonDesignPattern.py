import threading
from typing import List, Optional


class Logger:
    _instance: Optional["Logger"] = None
    _lock = threading.RLock()

    def __init__(self):
        print("object created")
        self.logs: List[str] = []

    def append_logs(self, log: str):
        self.logs.append(log)

    @classmethod
    def get_instance_normal_thread_unsafe(cls) -> "Logger":
        if cls._instance is None:
            cls._instance = Logger()
        return cls._instance

    @classmethod
    def get_instance_synchronized(cls) -> "Logger":
        with cls._lock:
            if cls._instance is None:
                cls._instance = Logger()
            return cls._instance

    @classmethod
    def get_instance(cls) -> "Logger":
        if cls._instance is None:
            with cls._lock:
                if cls._instance is None:
                    cls._instance = Logger()
        return cls._instance


def main():
    # logger1 = Logger.get_instance()
    # logger2 = Logger.get_instance()
    # logger3 = Logger.get_instance()

    def task1():
        Logger.get_instance()

    # thread1 = threading.Thread(target=task1)
    # thread2 = threading.Thread(target=task1)
    # thread1.start()
    # thread2.start()

    for _ in range(2):
        threading.Thread(target=task1).start()


if __name__ == "__main__":
    main()
