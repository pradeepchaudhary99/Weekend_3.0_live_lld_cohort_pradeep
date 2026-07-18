from __future__ import annotations

import time
from abc import ABC, abstractmethod
from collections import deque
from typing import Deque, Dict


class RateLimiter(ABC):
    @abstractmethod
    def is_allowed(self, user_id: str) -> bool:
        ...


class TokenBucket:
    def __init__(self, capacity: int = 10, refill_rate: float = 1 / 3600):
        self.capacity = capacity
        self.tokens = capacity
        self.refill_rate = refill_rate  # 1 token / 3600 seconds
        self.last_refill_time = time.time()

    def _refill(self) -> None:
        current_time = time.time()
        diff = current_time - self.last_refill_time
        eligible_tokens = int(diff * self.refill_rate)
        self.tokens = min(self.capacity, self.tokens + eligible_tokens)
        self.last_refill_time = current_time

    def check_if_allowed(self) -> bool:
        self._refill()
        if self.tokens >= 1:
            self.tokens -= 1
            return True
        return False


class TokenBucketRateLimitingAlgorithm(RateLimiter):
    def __init__(self):
        self.token_buckets: Dict[str, TokenBucket] = {}

    def is_allowed(self, user_id: str) -> bool:
        user_tb = self.token_buckets.setdefault(user_id, TokenBucket())
        return user_tb.check_if_allowed()


# Per User Sliding Windows
class SlidingWindow:
    def __init__(self, window_size_seconds: int = 60, maximum_request_allowed_in_window: int = 5):
        self.queue: Deque[int] = deque()
        self.window_size = window_size_seconds
        self.maximum_request_allowed_in_window = maximum_request_allowed_in_window

    def is_allowed(self) -> bool:
        current_time = int(time.time())
        if len(self.queue) < self.maximum_request_allowed_in_window:
            self.queue.append(current_time)
            return True

        while self.queue and current_time - self.window_size > self.queue[0]:
            self.queue.popleft()

        if len(self.queue) < self.maximum_request_allowed_in_window:
            self.queue.append(current_time)
            return True
        return False


# Q: Removing from front, inserting at the end....
class SlidingWindowStrategy(RateLimiter):
    def __init__(self):
        self.windows: Dict[str, SlidingWindow] = {}

    def is_allowed(self, user_id: str) -> bool:
        window = self.windows.setdefault(user_id, SlidingWindow())
        return window.is_allowed()


class RateLimiterDemo:
    pass


if __name__ == "__main__":
    pass
