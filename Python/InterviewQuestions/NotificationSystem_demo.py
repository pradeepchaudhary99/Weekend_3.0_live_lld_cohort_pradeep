"""
FunctionalRequirements:

Users should be able to send Notification
System should be able to support multiple types of channels
    --> Email
    --> SMS
    --> Push
    --> Whatsapp
    --> Slack
Notification System should care about user preference
Notification sending should be asynchronous
Easy to add new notification types/channels


Entities:

User --> Client ---> main --> class Clients
            // SendNotification

Notification
NotificationService
NotificationChannel
UserPreferencesRepository
NotificationChannelFactory
"""

from __future__ import annotations

from abc import ABC, abstractmethod
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass
from enum import Enum, auto
from typing import Dict, Set


@dataclass
class Notification:
    recipient: str
    title: str
    message: str


class NotificationChannel(ABC):
    @abstractmethod
    def send_notification(self, notification: Notification) -> None:
        ...


class NotificationDecorator(NotificationChannel):
    def __init__(self, notification_channel: NotificationChannel):
        self.notification_channel = notification_channel


class RateLimiterDecorator(NotificationDecorator):
    def send_notification(self, notification: Notification) -> None:
        # 1000 lines of rate limiting logic will be here
        self.notification_channel.send_notification(notification)


class SMSNotification(NotificationChannel):
    def send_notification(self, notification: Notification) -> None:
        pass


class PushNotification(NotificationChannel):
    def send_notification(self, notification: Notification) -> None:
        pass


class WhatsappNotification(NotificationChannel):
    def send_notification(self, notification: Notification) -> None:
        pass


class EmailNotification(NotificationChannel):
    def send_notification(self, notification: Notification) -> None:
        raise NotImplementedError("send_notification")


class NotificationType(Enum):
    SMS = auto()
    PUSH = auto()
    EMAIL = auto()
    WHATSAPP = auto()


class NotificationFactory:
    def __init__(self):
        self.registry: Dict[NotificationType, NotificationChannel] = {
            NotificationType.EMAIL: EmailNotification(),
            NotificationType.SMS: EmailNotification(),
            NotificationType.WHATSAPP: EmailNotification(),
        }

    def get_instance(self, notification_type: NotificationType) -> NotificationChannel:
        return self.registry[notification_type]


class UserPreferencesRepository:
    def __init__(self):
        self.user_preferences: Dict[str, Set[NotificationChannel]] = {}

    def get_user_preference(self, user_id: str) -> Set[NotificationChannel]:
        return self.user_preferences.get(user_id, set())


# Orchestrator, Facade
class NotificationService:
    def __init__(self):
        self.executor = ThreadPoolExecutor(max_workers=4)
        self.factory = NotificationFactory()
        self.user_preferences_repository = UserPreferencesRepository()

    def send(self, notification: Notification) -> None:
        preferred_channels = self.user_preferences_repository.get_user_preference(notification.recipient)
        for channel in preferred_channels:
            # SMS, Whatsapp, Gmail
            # Async
            self.executor.submit(channel.send_notification, notification)
            # channel.send_notification(notification)  # sync


# Applying Decorator Design Pattern:
# Apply Decorator design pattern to enforce Retry Logic, Rate limiting, Formatting
class NotificationClient:
    def send_notification(self, notification_service: NotificationService, notification: Notification) -> None:
        pass


class NotificationSystemDemo:
    pass


if __name__ == "__main__":
    pass
