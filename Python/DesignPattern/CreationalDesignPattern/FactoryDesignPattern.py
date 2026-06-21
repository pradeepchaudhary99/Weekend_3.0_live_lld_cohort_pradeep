# Factory Design Pattern
from abc import ABC, abstractmethod


class INotification(ABC):
    @abstractmethod
    def send(self): pass


class SMSNotification(INotification):
    def send(self): print("Sending SMS notification")


class WhatsAppNotification(INotification):
    def send(self): print("Sending WhatsApp notification")


class SlackNotification(INotification):
    def send(self): print("Sending Slack notification")


class NotificationFactory:
    _registry: dict = {}

    @staticmethod
    def get_instance(type: str) -> INotification:
        if type == "SMS":
            NotificationFactory._registry.setdefault(type, SMSNotification())
        elif type == "whatsapp":
            NotificationFactory._registry.setdefault(type, WhatsAppNotification())
        elif type == "Slack":
            NotificationFactory._registry.setdefault(type, SlackNotification())
        return NotificationFactory._registry.get(type)


class NotificationService:
    def send_notification(self, type: str, message: str):
        notification = NotificationFactory.get_instance(type)
        if notification:
            notification.send()


if __name__ == "__main__":
    service = NotificationService()
    service.send_notification("SMS", "Hello!")
    service.send_notification("whatsapp", "Hello!")
    service.send_notification("Slack", "Hello!")
