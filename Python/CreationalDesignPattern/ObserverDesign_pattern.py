from abc import ABC, abstractmethod
from typing import List


class Subject(ABC):
    @abstractmethod
    def add_observer(self, observer):
        pass

    @abstractmethod
    def remove_observer(self, observer):
        pass


class Follower(ABC):
    @abstractmethod
    def notify(self, message: str):
        pass


class WhatsAppBroadCast:
    def __init__(self):
        self.followers: List[Follower] = []

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


def main():
    whats_app_broad_cast = WhatsAppBroadCast()
    whats_app_broad_cast.add_follower(Prateek())
    whats_app_broad_cast.add_follower(Abhinav())

    whats_app_broad_cast.send_message("pradeep is teaching Observer pattern")


if __name__ == "__main__":
    main()
