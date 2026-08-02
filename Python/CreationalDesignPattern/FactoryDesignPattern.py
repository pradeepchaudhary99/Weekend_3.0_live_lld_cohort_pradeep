from abc import ABC, abstractmethod
from typing import Dict, Optional


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


# Problem
# class NotificationService:
#     def send_notification(self, type, message):
#         notification = None
#         if type == "SMS":
#             notification = SMSNotification()
#         elif type == "whatsapp":
#             notification = WhatsAppNotification()


class NotificationService:
    def send_notification(self, type: str, message: str):
        notification = NotificationFactory.get_instance(type)
        # validation
        # 10000
        notification.send()


class NotificationFactory:
    registry: Dict[str, INotification] = {}

    @staticmethod
    def get_instance(type: str) -> Optional[INotification]:
        if type == "SMS":
            NotificationFactory.registry.setdefault(type, SMSNotification())
        elif type == "whatsapp":
            NotificationFactory.registry.setdefault(type, WhatsAppNotification())
        elif type == "Slack":
            pass
        return NotificationFactory.registry.get(type)


class FactoryDesignPattern:
    pass
