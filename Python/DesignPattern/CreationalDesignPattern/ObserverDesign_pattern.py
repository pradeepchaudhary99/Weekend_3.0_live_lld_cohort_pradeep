from abc import ABC, abstractmethod


class Subject(ABC):
    @abstractmethod
    def add_observer(self, observer): pass

    @abstractmethod
    def remove_observer(self, observer): pass


class Follower(ABC):
    @abstractmethod
    def notify(self, message: str): pass


class WhatsAppBroadCast:
    def __init__(self):
        self.followers = []

    def add_follower(self, follower: Follower):
        self.followers.append(follower)

    def remove_follower(self, follower: Follower):
        self.followers.remove(follower)

    def send_message(self, message: str):
        for follower in self.followers:
            follower.notify(message)


class Prateek(Follower):
    def notify(self, message: str):
        print(f" Prateek Received Message: {message}")


class Abhinav(Follower):
    def notify(self, message: str):
        print(f"Abhinav Received Message: {message}")


if __name__ == "__main__":
    whatsapp_broadcast = WhatsAppBroadCast()
    whatsapp_broadcast.add_follower(Prateek())
    whatsapp_broadcast.add_follower(Abhinav())

    whatsapp_broadcast.send_message("pradeep is teaching Observer pattern")
