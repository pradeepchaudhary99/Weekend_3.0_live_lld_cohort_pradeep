# OrderProcessor — SOLID violations demo
# Shows bad patterns and refactored good patterns

from abc import ABC, abstractmethod


# =======================
# BAD: OrderProcessor violates SRP, OCP, DIP
# =======================
class OrderProcessor:
    def process_order(self, customer_type: str, items: list, payment_method: str):

        # 1. Calculate total
        total = 0
        for item in items:
            if item == "LAPTOP":      total += 80000
            elif item == "PHONE":     total += 30000
            elif item == "HEADPHONES": total += 5000

        # 2. Apply discount — hardcoded (OCP violation)
        if customer_type == "PREMIUM":
            total *= 0.80
        elif customer_type == "REGULAR":
            total *= 0.95

        # 3. Process payment — hardcoded (OCP violation)
        if payment_method == "CREDIT_CARD":
            print(f"Processing credit card payment of ₹{total}")
        elif payment_method == "UPI":
            print(f"Processing UPI payment of ₹{total}")
        elif payment_method == "NETBANKING":
            print(f"Processing NetBanking payment of ₹{total}")

        # 4. Save to DB (SRP violation)
        print("Saving order to database...")

        # 5. Send email (SRP violation)
        print("Sending confirmation email to customer...")

        # 6. Generate invoice (SRP violation)
        print("=== INVOICE ===")
        print(f"Items: {items}")
        print(f"Total after discount: ₹{total}")
        print(f"Payment via: {payment_method}")


# =======================
# BAD: LSP Violation
# =======================
class Animal:
    def sound(self) -> str:
        return "Some sound"

    def fly(self) -> str:
        return "I can fly!"

class Dog(Animal):
    def fly(self) -> str:
        raise NotImplementedError("Dogs can't fly!")  # LSP violation

class Bird(Animal):
    def fly(self) -> str:
        return "I am flying!"


# =======================
# BAD: ISP Violation
# =======================
class WorkerInterface(ABC):
    @abstractmethod
    def work(self): pass
    @abstractmethod
    def eat(self): pass
    @abstractmethod
    def sleep(self): pass
    @abstractmethod
    def attend_meeting(self): pass
    @abstractmethod
    def write_code(self): pass

class HumanWorker(WorkerInterface):
    def work(self): print("Human working")
    def eat(self): print("Human eating")
    def sleep(self): print("Human sleeping")
    def attend_meeting(self): print("Human in meeting")
    def write_code(self): print("Human writing code")

class RobotWorker(WorkerInterface):
    def work(self): print("Robot working")
    def eat(self): raise NotImplementedError("Robots don't eat!")   # ISP violation
    def sleep(self): raise NotImplementedError("Robots don't sleep!") # ISP violation
    def attend_meeting(self): print("Robot in meeting")
    def write_code(self): print("Robot writing code")


# =======================
# GOOD: DIP — depend on abstraction
# =======================
class IDatabase(ABC):
    @abstractmethod
    def save(self, data: str): pass

class MySQLDatabase(IDatabase):
    def save(self, data: str):
        print(f"Saving to MySQL: {data}")

class NoSQLDatabase(IDatabase):
    def save(self, data: str):
        print(f"Saving to NoSQL: {data}")

class ReportService:
    def __init__(self, database: IDatabase):
        self.database = database

    def set_database(self, database: IDatabase):
        self.database = database

    def generate_report(self, data: str):
        print("Generating report...")
        self.database.save(data)


if __name__ == "__main__":
    processor = OrderProcessor()
    processor.process_order("PREMIUM", ["LAPTOP", "PHONE"], "UPI")

    report = ReportService(MySQLDatabase())
    report.generate_report("Order #1001")

    report.set_database(NoSQLDatabase())
    report.generate_report("Order #1002")
