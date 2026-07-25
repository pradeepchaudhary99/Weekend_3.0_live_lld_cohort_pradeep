"""
Rate Limiter
------------------------------

Functional Requirements:
    System should limit the number of requests a user can make in a given time
    Support multiple algorithms: Token Bucket, Sliding Window
    Rate limiting should be applied per user (per key)
    Easy to add new algorithms

Non-Functional Requirements:
    Thread-safe: many threads can call is_allowed() for the same/different users concurrently
    Low latency: no global lock across all users
    Extensible: new strategies pluggable via the RateLimiter interface (Strategy pattern)

Entities:
    RateLimiter (Strategy interface)
    TokenBucket / TokenBucketRateLimiter
    SlidingWindow / SlidingWindowRateLimiter
    RateLimiterFactory
"""

import threading
import time
from abc import ABC, abstractmethod
from collections import deque
from concurrent.futures import ThreadPoolExecutor
from enum import Enum, auto
from typing import Deque, Dict


class RateLimiter(ABC):
    @abstractmethod
    def is_allowed(self, user_id: str) -> bool:
        raise NotImplementedError


class TokenBucket:
    """One bucket per user. A per-bucket lock keeps refill + consume atomic."""

    def __init__(self, capacity: int, refill_tokens_per_second: float):
        self._capacity = capacity
        self._refill_tokens_per_second = refill_tokens_per_second
        self._tokens = float(capacity)
        self._last_refill_time = time.monotonic()
        self._lock = threading.Lock()

    def _refill(self) -> None:
        now = time.monotonic()
        elapsed_seconds = now - self._last_refill_time
        eligible_tokens = elapsed_seconds * self._refill_tokens_per_second
        if eligible_tokens > 0:
            self._tokens = min(self._capacity, self._tokens + eligible_tokens)
            self._last_refill_time = now

    def try_consume(self) -> bool:
        with self._lock:
            self._refill()
            if self._tokens >= 1.0:
                self._tokens -= 1.0
                return True
            return False


class TokenBucketRateLimiter(RateLimiter):
    """Per-user buckets are created lazily; the registry lock only guards
    bucket creation, never the (much hotter) consume path."""

    def __init__(self, capacity: int, refill_tokens_per_second: float):
        self._capacity = capacity
        self._refill_tokens_per_second = refill_tokens_per_second
        self._buckets: Dict[str, TokenBucket] = {}
        self._registry_lock = threading.Lock()

    def _get_or_create_bucket(self, user_id: str) -> TokenBucket:
        bucket = self._buckets.get(user_id)
        if bucket is None:
            with self._registry_lock:
                bucket = self._buckets.get(user_id)
                if bucket is None:
                    bucket = TokenBucket(self._capacity, self._refill_tokens_per_second)
                    self._buckets[user_id] = bucket
        return bucket

    def is_allowed(self, user_id: str) -> bool:
        return self._get_or_create_bucket(user_id).try_consume()


class SlidingWindow:
    """One window per user; its own lock keeps the timestamp deque consistent."""

    def __init__(self, window_size_millis: int, max_requests_per_window: int):
        self._timestamps: Deque[int] = deque()
        self._window_size_millis = window_size_millis
        self._max_requests_per_window = max_requests_per_window
        self._lock = threading.Lock()

    def is_allowed(self) -> bool:
        with self._lock:
            now = int(time.time() * 1000)
            while self._timestamps and now - self._timestamps[0] > self._window_size_millis:
                self._timestamps.popleft()
            if len(self._timestamps) < self._max_requests_per_window:
                self._timestamps.append(now)
                return True
            return False


class SlidingWindowRateLimiter(RateLimiter):
    def __init__(self, window_size_millis: int, max_requests_per_window: int):
        self._window_size_millis = window_size_millis
        self._max_requests_per_window = max_requests_per_window
        self._windows: Dict[str, SlidingWindow] = {}
        self._registry_lock = threading.Lock()

    def _get_or_create_window(self, user_id: str) -> SlidingWindow:
        window = self._windows.get(user_id)
        if window is None:
            with self._registry_lock:
                window = self._windows.get(user_id)
                if window is None:
                    window = SlidingWindow(self._window_size_millis, self._max_requests_per_window)
                    self._windows[user_id] = window
        return window

    def is_allowed(self, user_id: str) -> bool:
        return self._get_or_create_window(user_id).is_allowed()


class RateLimiterType(Enum):
    TOKEN_BUCKET = auto()
    SLIDING_WINDOW = auto()


class RateLimiterFactory:
    """Factory pattern: callers pick an algorithm without knowing the concrete class."""

    @staticmethod
    def create(limiter_type: RateLimiterType, limit: int, window_or_refill_millis: int) -> RateLimiter:
        if limiter_type == RateLimiterType.TOKEN_BUCKET:
            refill_per_second = limit / (window_or_refill_millis / 1000.0)
            return TokenBucketRateLimiter(limit, refill_per_second)
        if limiter_type == RateLimiterType.SLIDING_WINDOW:
            return SlidingWindowRateLimiter(window_or_refill_millis, limit)
        raise ValueError(f"Unsupported rate limiter type: {limiter_type}")


def main() -> None:
    # 5 requests allowed per 1-second window, refilling continuously.
    rate_limiter = RateLimiterFactory.create(RateLimiterType.TOKEN_BUCKET, 5, 1000)

    thread_count = 10
    user_id = "user-1"
    print(f"Firing {thread_count} concurrent requests for {user_id} "
          f"against a token bucket of capacity 5...")

    allowed = 0
    rejected = 0
    lock = threading.Lock()

    def worker() -> None:
        nonlocal allowed, rejected
        if rate_limiter.is_allowed(user_id):
            with lock:
                allowed += 1
        else:
            with lock:
                rejected += 1

    with ThreadPoolExecutor(max_workers=thread_count) as executor:
        futures = [executor.submit(worker) for _ in range(thread_count)]
        for future in futures:
            future.result()

    print(f"Allowed: {allowed}, Rejected: {rejected}")

    print("\nSliding window demo (max 3 requests / 1000ms):")
    sliding_window_limiter = RateLimiterFactory.create(RateLimiterType.SLIDING_WINDOW, 3, 1000)
    for i in range(1, 6):
        print(f"Request {i} allowed = {sliding_window_limiter.is_allowed('user-2')}")


if __name__ == "__main__":
    main()
