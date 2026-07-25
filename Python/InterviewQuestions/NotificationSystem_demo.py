"""
FunctionalRequirements:

Users should be able to send Notification
System should support multiple types of channels
    --> Email
    --> SMS
    --> Push
    --> Whatsapp

Notification System should care about user preference
Notification sending should be asynchronous
Easy to add new notification types/channels
Notifications should be resilient to transient channel failures (retry)
Notifications should be rate-limited per recipient per channel


Non-Functional Requirements:
Thread-safe: preferences and channel state are shared across worker threads
Async: sending never blocks the caller
Extensible: new channels/behaviors pluggable via Factory + Decorator


Entities:

User --> Client ---> main --> class Clients
            // SendNotification

Notification
NotificationService
NotificationChannel
UserPreferencesRepository
NotificationChannelFactory

Applying Decorator Design Pattern:
    RateLimiterDecorator, RetryDecorator wrap any NotificationChannel to add
    cross-cutting behavior without touching the concrete channel classes.
"""

from __future__ import annotations

import threading
import time
from abc import ABC, abstractmethod
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass
from enum import Enum, auto
from typing import Dict, Set


@dataclass(frozen=True)
class Notification:
    recipient: str
    title: str
    message: str


class NotificationDeliveryError(RuntimeError):
    pass


class NotificationChannel(ABC):
    @abstractmethod
    def send_notification(self, notification: Notification) -> None:
        raise NotImplementedError


# ---------- Concrete channels ----------

class SMSNotification(NotificationChannel):
    def send_notification(self, notification: Notification) -> None:
        print(f"[SMS] to {notification.recipient}: {notification.title}")


class PushNotification(NotificationChannel):
    def send_notification(self, notification: Notification) -> None:
        print(f"[PUSH] to {notification.recipient}: {notification.title}")


class WhatsappNotification(NotificationChannel):
    def send_notification(self, notification: Notification) -> None:
        print(f"[WHATSAPP] to {notification.recipient}: {notification.title}")


class EmailNotification(NotificationChannel):
    def send_notification(self, notification: Notification) -> None:
        print(f"[EMAIL] to {notification.recipient}: {notification.title}")


# ---------- Decorators (Decorator pattern) ----------

class NotificationDecorator(NotificationChannel):
    def __init__(self, notification_channel: NotificationChannel):
        self.notification_channel = notification_channel


class RateLimiterDecorator(NotificationDecorator):
    """Fixed-window counter per recipient: at most `limit` sends per `window_seconds`.
    Thread-safe: a lock guards each recipient's window state."""

    class _Window:
        __slots__ = ("count", "window_start", "lock")

        def __init__(self):
            self.count = 0
            self.window_start = time.monotonic()
            self.lock = threading.Lock()

    def __init__(self, notification_channel: NotificationChannel, limit: int, window_seconds: float):
        super().__init__(notification_channel)
        self._limit = limit
        self._window_seconds = window_seconds
        self._windows: Dict[str, RateLimiterDecorator._Window] = {}
        self._registry_lock = threading.Lock()

    def _get_window(self, recipient: str) -> "RateLimiterDecorator._Window":
        window = self._windows.get(recipient)
        if window is None:
            with self._registry_lock:
                window = self._windows.get(recipient)
                if window is None:
                    window = RateLimiterDecorator._Window()
                    self._windows[recipient] = window
        return window

    def send_notification(self, notification: Notification) -> None:
        window = self._get_window(notification.recipient)
        with window.lock:
            now = time.monotonic()
            if now - window.window_start > self._window_seconds:
                window.window_start = now
                window.count = 0
            window.count += 1
            if window.count > self._limit:
                print(f"[RateLimited] dropping notification to {notification.recipient}")
                return
        self.notification_channel.send_notification(notification)


class RetryDecorator(NotificationDecorator):
    """Retries the wrapped channel with a short backoff on transient failures."""

    def __init__(self, notification_channel: NotificationChannel, max_attempts: int, backoff_seconds: float):
        super().__init__(notification_channel)
        self._max_attempts = max_attempts
        self._backoff_seconds = backoff_seconds

    def send_notification(self, notification: Notification) -> None:
        last_failure: Exception = None
        for attempt in range(1, self._max_attempts + 1):
            try:
                self.notification_channel.send_notification(notification)
                return
            except NotificationDeliveryError as e:
                last_failure = e
                print(f"Attempt {attempt} failed for {notification.recipient}: {e}")
                time.sleep(self._backoff_seconds)
        print(f"Giving up on notification to {notification.recipient} after {self._max_attempts} attempts")
        if last_failure is not None:
            raise last_failure


# ---------- Factory (Factory pattern) ----------

class NotificationType(Enum):
    SMS = auto()
    PUSH = auto()
    EMAIL = auto()
    WHATSAPP = auto()


class NotificationChannelFactory:
    def __init__(self):
        self._registry: Dict[NotificationType, NotificationChannel] = {
            NotificationType.EMAIL: self._decorate(EmailNotification()),
            NotificationType.SMS: self._decorate(SMSNotification()),
            NotificationType.PUSH: self._decorate(PushNotification()),
            NotificationType.WHATSAPP: self._decorate(WhatsappNotification()),
        }

    @staticmethod
    def _decorate(base: NotificationChannel) -> NotificationChannel:
        # Every channel gets retry-on-failure and per-recipient rate limiting for free.
        return RetryDecorator(RateLimiterDecorator(base, limit=3, window_seconds=1.0), max_attempts=2, backoff_seconds=0.05)

    def get_instance(self, notification_type: NotificationType) -> NotificationChannel:
        channel = self._registry.get(notification_type)
        if channel is None:
            raise ValueError(f"No channel registered for type {notification_type}")
        return channel


# ---------- User preferences ----------

class UserPreferencesRepository:
    def __init__(self):
        self._user_preferences: Dict[str, Set[NotificationType]] = {}
        self._lock = threading.Lock()

    def set_user_preference(self, user_id: str, types: Set[NotificationType]) -> None:
        with self._lock:
            self._user_preferences[user_id] = set(types)

    def get_user_preference(self, user_id: str) -> Set[NotificationType]:
        with self._lock:
            return self._user_preferences.get(user_id, {NotificationType.EMAIL})


# Orchestrator / Facade
class NotificationService:
    def __init__(self, user_preferences_repository: UserPreferencesRepository):
        self._executor = ThreadPoolExecutor(max_workers=4)
        self._factory = NotificationChannelFactory()
        self._user_preferences_repository = user_preferences_repository

    def send(self, notification: Notification) -> None:
        preferred_types = self._user_preferences_repository.get_user_preference(notification.recipient)
        for notification_type in preferred_types:
            channel = self._factory.get_instance(notification_type)
            # Async: fan out to every preferred channel without blocking the caller.
            self._executor.submit(channel.send_notification, notification)

    def shutdown(self) -> None:
        self._executor.shutdown(wait=True)


class NotificationClient:
    def send_notification(self, notification_service: NotificationService, notification: Notification) -> None:
        notification_service.send(notification)


def main() -> None:
    preferences = UserPreferencesRepository()
    preferences.set_user_preference("alice@example.com", {NotificationType.EMAIL, NotificationType.SMS})
    preferences.set_user_preference("bob@example.com", {NotificationType.PUSH, NotificationType.WHATSAPP})

    service = NotificationService(preferences)
    client = NotificationClient()

    with ThreadPoolExecutor(max_workers=2) as callers:
        futures = []
        for i in range(3):
            futures.append(callers.submit(
                client.send_notification, service,
                Notification("alice@example.com", f"Order Update {i}", "Your order shipped")))
            futures.append(callers.submit(
                client.send_notification, service,
                Notification("bob@example.com", f"Promo {i}", "50% off today")))
        for future in futures:
            future.result()

    service.shutdown()
    print("All notifications dispatched.")


if __name__ == "__main__":
    main()
