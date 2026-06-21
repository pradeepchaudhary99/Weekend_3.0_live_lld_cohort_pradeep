# Notification system — Dependency Injection via composition
from abc import ABC, abstractmethod


class INotification(ABC):
    @abstractmethod
    def send_notification(self, message: str): pass


class SMSNotification(INotification):
    def send_notification(self, message: str):
        print(f"SMS: {message}")


class EmailNotification(INotification):
    def send_notification(self, message: str):
        print(f"Email: {message}")


class WhatsAppNotification(INotification):
    def send_notification(self, message: str):
        print(f"WhatsApp: {message}")


class SlackNotification(INotification):
    def send_notification(self, message: str):
        print(f"Slack: {message}")


class NotificationClient:
    def __init__(self):
        self.notification: INotification = SMSNotification()

    def set_notification(self, notification: INotification):
        self.notification = notification

    def send_notification(self, message: str):
        self.notification.send_notification(message)


if __name__ == "__main__":
    client = NotificationClient()
    client.send_notification("Hello via SMS")

    client.set_notification(SlackNotification())
    client.send_notification("Hello via Slack")
