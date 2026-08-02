from abc import ABC, abstractmethod
from typing import Dict, Optional

# fun non
# entitieis
#
# relationsji
#
# class Notification
#     - priority
#     - message


class IRateLimitinStrategy(ABC):
    pass


class Notification:
    def __init__(self):
        self.priority: Optional[str] = None
        self.message: Optional[str] = None


class Database(ABC):
    @abstractmethod
    def fetch_data(self, query: str) -> str:
        pass


class RealDatabase(Database):
    def fetch_data(self, query: str) -> str:
        print("Fetching confidential employee salary data...")
        return f"data for {query}"


class DatabaseProxy(Database):
    def __init__(self, role: Optional[str]):
        self.role = role
        self.database: Optional[RealDatabase] = None
        self.cache: Dict[str, str] = {}

    def fetch_data(self, query: str) -> str:
        if query in self.cache:
            return self.cache[query]

        if self.database is None:
            self.database = RealDatabase()

        value = self.database.fetch_data(query)
        self.cache[query] = value
        return value


def main():
    user = DatabaseProxy(None)
    user.fetch_data("dsadsadasd")


if __name__ == "__main__":
    main()
