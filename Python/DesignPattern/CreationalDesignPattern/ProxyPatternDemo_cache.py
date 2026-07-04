from abc import ABC, abstractmethod


class IRateLimitinStrategy(ABC):
    pass


class Notification:
    def __init__(self, priority: str = "", message: str = ""):
        self.priority = priority
        self.message = message


class Database(ABC):
    @abstractmethod
    def fetch_data(self, query: str) -> str: pass


class RealDatabase(Database):
    def fetch_data(self, query: str) -> str:
        print("Fetching confidential employee salary data...")
        return f"result for '{query}'"


class DatabaseProxy(Database):
    def __init__(self, role: str):
        self.role = role
        self.database = None
        self.cache = {}

    def fetch_data(self, query: str) -> str:
        if query in self.cache:
            return self.cache[query]

        if self.database is None:
            self.database = RealDatabase()

        value = self.database.fetch_data(query)
        self.cache[query] = value
        return value


if __name__ == "__main__":
    user = DatabaseProxy(None)
    print(user.fetch_data("dsadsadasd"))
