"""Chain of Responsibility Design Pattern - Log Handling Example"""

from abc import ABC, abstractmethod
from enum import Enum
from typing import Optional


class LogLevel(Enum):
    DEBUG = 0
    INFO = 1
    WARN = 2
    ERROR = 3
    FATAL = 4

    @property
    def severity(self) -> int:
        return self.value


# Handlers
# Chain
# Request
class Log:
    def __init__(self, message: str, level: LogLevel):
        self.message = message
        self.level = level

    def get_log_level(self) -> LogLevel:
        return self.level

    def __str__(self) -> str:
        return f"[{self.level.name}] {self.message}"


class Handler(ABC):
    def __init__(self, next_handler: Optional["Handler"]):
        self.next_handler = next_handler

    def call_next_handler(self, log: Log):
        if self.next_handler:
            self.next_handler.handle(log)

    @abstractmethod
    def can_handle(self, log: Log) -> bool:
        pass

    @abstractmethod
    def handle(self, log: Log):
        pass


class Level1(Handler):
    def __init__(self, next_handler: Optional[Handler]):
        super().__init__(next_handler)

    def can_handle(self, log: Log) -> bool:
        return log.get_log_level().severity <= LogLevel.INFO.severity

    def handle(self, log: Log):
        if self.can_handle(log):
            print("Reqeust is handled by Level1")
            print(f"Logging the log{log}")
        else:
            self.call_next_handler(log)


class Level2(Handler):
    def __init__(self, next_handler: Optional[Handler]):
        super().__init__(next_handler)

    def can_handle(self, log: Log) -> bool:
        return log.get_log_level().severity == LogLevel.WARN.severity

    def handle(self, log: Log):
        if self.can_handle(log):
            print("Reqeust is handled by Level1")
            print(f"Logging the log{log}")
        else:
            self.call_next_handler(log)


class Level3(Handler):
    def __init__(self, next_handler: Optional[Handler]):
        super().__init__(next_handler)

    def can_handle(self, log: Log) -> bool:
        return log.get_log_level().severity >= LogLevel.ERROR.severity

    def handle(self, log: Log):
        if self.can_handle(log):
            print("Reqeust is handled by Level1")
            print(f"Logging the log{log}")
        else:
            self.call_next_handler(log)


if __name__ == "__main__":
    level3 = Level3(None)
    level2 = Level2(level3)
    level1 = Level1(level2)

    level1.handle(Log("log this", LogLevel.DEBUG))
