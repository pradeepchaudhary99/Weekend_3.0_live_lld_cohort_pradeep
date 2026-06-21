# Factory Method Design Pattern
from abc import ABC, abstractmethod


# Product interface
class INotification(ABC):
    @abstractmethod
    def send(self): pass


class SMSNotification(INotification):
    def send(self): print("Sending SMS notification")


class WhatsAppNotification(INotification):
    def send(self): print("Sending WhatsApp notification")


class SlackNotification(INotification):
    def send(self): print("Sending Slack notification")


# Factory interface
class INotificationFactory(ABC):
    @abstractmethod
    def get_notification(self) -> INotification: pass


class SMSNotificationFactory(INotificationFactory):
    def get_notification(self) -> INotification:
        return SMSNotification()


class WhatsAppNotificationFactory(INotificationFactory):
    def get_notification(self) -> INotification:
        return WhatsAppNotification()


class SlackNotificationFactory(INotificationFactory):
    def get_notification(self) -> INotification:
        return SlackNotification()


class NotificationService:
    def send_notification(self, factory: INotificationFactory):
        notification = factory.get_notification()
        notification.send()


if __name__ == "__main__":
    service = NotificationService()
    service.send_notification(SMSNotificationFactory())
    service.send_notification(WhatsAppNotificationFactory())
    service.send_notification(SlackNotificationFactory())
