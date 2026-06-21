# Singleton Design Pattern
import threading


class Logger:
    _instance = None
    _lock = threading.Lock()

    def __init__(self):
        print("object created")
        self.logs = []

    def append_logs(self, log: str):
        self.logs.append(log)

    # Thread-unsafe version
    @staticmethod
    def get_instance_thread_unsafe():
        if Logger._instance is None:
            Logger._instance = Logger()
        return Logger._instance

    # Thread-safe: synchronized (lock on every call)
    @staticmethod
    def get_instance_synchronized():
        with Logger._lock:
            if Logger._instance is None:
                Logger._instance = Logger()
        return Logger._instance

    # Thread-safe: double-checked locking
    @staticmethod
    def get_instance():
        if Logger._instance is None:
            with Logger._lock:
                if Logger._instance is None:
                    Logger._instance = Logger()
        return Logger._instance


if __name__ == "__main__":
    def task():
        Logger.get_instance()

    threads = [threading.Thread(target=task) for _ in range(2)]
    for t in threads:
        t.start()
    for t in threads:
        t.join()
