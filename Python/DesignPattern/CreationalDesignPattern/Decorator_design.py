from abc import ABC, abstractmethod


# Product interface
class INotification(ABC):
    @abstractmethod
    def send(self, message: str): pass


class SMSNotification(INotification):
    def send(self, message: str):
        print(f"Sending SMS notification {message}")


# Base decorator
class NotificationDecorator(INotification, ABC):
    def __init__(self, notification: INotification):
        self._base_notification = notification


class FormatNotification(NotificationDecorator):
    def send(self, message: str):
        print("Notification is formatted")
        self._base_notification.send(message)


class AnimationNotification(NotificationDecorator):
    def send(self, message: str):
        print("AnimationNotification is added")
        self._base_notification.send(message)


if __name__ == "__main__":
    notification: INotification = SMSNotification()
    notification.send("pradeeep")
