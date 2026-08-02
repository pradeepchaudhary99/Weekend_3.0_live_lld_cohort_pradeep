from abc import ABC, abstractmethod


# Product Interface
class INotification(ABC):
    @abstractmethod
    def send(self):
        pass


class SMSNotification(INotification):
    def send(self):
        raise NotImplementedError("Unimplemented method 'send'")


class WhatsAppNotification(INotification):
    def send(self):
        raise NotImplementedError("Unimplemented method 'send'")


class INotificationFactory(ABC):
    @abstractmethod
    def get_notification(self) -> INotification:
        pass


class SMSNotificationFactory(INotificationFactory):
    def get_notification(self) -> INotification:
        return SMSNotification()


class WhatsAppNotificationFactory(INotificationFactory):
    def get_notification(self) -> INotification:
        return WhatsAppNotification()


class Slack(INotification):
    def send(self):
        pass


class SlackNotificationFactory(INotificationFactory):
    def get_notification(self) -> INotification:
        return Slack()


class NotificationService:
    def send_notification(self, factory: INotificationFactory):
        notification = factory.get_notification()
        notification.send()


def main():
    service = NotificationService()
    service.send_notification(SMSNotificationFactory())


if __name__ == "__main__":
    main()
