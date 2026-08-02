from abc import ABC, abstractmethod


# Product
class INotification(ABC):
    @abstractmethod
    def send(self, message: str):
        pass


class SMSNotification(INotification):
    def send(self, message: str):
        print(f"Sending SMS notification {message}")


class NotificationDecorator(INotification, ABC):
    def __init__(self, notification: INotification):
        self.base_notification = notification


class FormatNotification(NotificationDecorator):
    def __init__(self, notification: INotification):
        super().__init__(notification)

    def send(self, message: str):
        print("Notification is formatted")
        self.base_notification.send(message)


class AnimationNotification(NotificationDecorator):
    def __init__(self, notification: INotification):
        super().__init__(notification)

    def send(self, message: str):
        print("AnimationNotification is added")
        self.base_notification.send(message)


def main():
    notification = SMSNotification()
    notification.send("pradeeep")


if __name__ == "__main__":
    main()
