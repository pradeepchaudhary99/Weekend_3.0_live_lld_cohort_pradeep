# SOLID Principles Demonstration

# =======================
# 1. SINGLE RESPONSIBILITY PRINCIPLE (SRP)
# =======================

# BAD: One class doing multiple things
class UserBad:
    def __init__(self, name):
        self.name = name

    def save_to_db(self):
        print("Saving user to DB")

    def send_email(self):
        print("Sending email")


# GOOD: Separate responsibilities
class User:
    def __init__(self, name):
        self.name = name

class UserRepository:
    def save(self, user: User):
        print("Saving user to DB")

class EmailService:
    def send_email(self, user: User):
        print("Sending email")


# =======================
# 2. OPEN CLOSED PRINCIPLE (OCP)
# =======================

# BAD: Need to modify class for new types
class DiscountCalculatorBad:
    def calculate(self, type: str) -> float:
        if type == "NEW":
            return 10
        elif type == "PREMIUM":
            return 20
        elif type == "DIWALI":
            return 30
        return 0


# GOOD: Extend without modifying
from abc import ABC, abstractmethod

class DiscountStrategy(ABC):
    @abstractmethod
    def calculate(self) -> float:
        pass

class NewCustomerDiscount(DiscountStrategy):
    def calculate(self) -> float:
        return 10

class PremiumCustomerDiscount(DiscountStrategy):
    def calculate(self) -> float:
        return 20

class DiwaliDiscount(DiscountStrategy):
    def calculate(self) -> float:
        return 30

class DiscountCalculator:
    def calculate(self, strategy: DiscountStrategy) -> float:
        return strategy.calculate()


# =======================
# 3. LISKOV SUBSTITUTION PRINCIPLE (LSP)
# =======================

# BAD: Violates substitution
class BirdBad:
    def fly(self):
        print("Flying")

class PenguinBad(BirdBad):
    def fly(self):
        raise NotImplementedError("Can't fly")


# GOOD: Proper abstraction
class Bird(ABC):
    @abstractmethod
    def eat(self): pass

    @abstractmethod
    def sleep(self): pass

class FlyingBird(Bird):
    @abstractmethod
    def fly(self): pass

class Sparrow(FlyingBird):
    def eat(self): print("Sparrow eating")
    def sleep(self): print("Sparrow sleeping")
    def fly(self): print("Sparrow flying")

class Penguin(Bird):
    def eat(self): print("Penguin eating")
    def sleep(self): print("Penguin sleeping")


# =======================
# 4. INTERFACE SEGREGATION PRINCIPLE (ISP)
# =======================

# BAD: Fat interface
class WorkerBad(ABC):
    @abstractmethod
    def work(self): pass

    @abstractmethod
    def eat(self): pass

class RobotBad(WorkerBad):
    def work(self): print("Working")
    def eat(self): raise NotImplementedError("Robot doesn't eat")


# GOOD: Split interfaces
class Workable(ABC):
    @abstractmethod
    def work(self): pass

class Eatable(ABC):
    @abstractmethod
    def eat(self): pass

class Human(Workable, Eatable):
    def work(self): print("Working")
    def eat(self): print("Eating")

class Robot(Workable):
    def work(self): print("Working")


# =======================
# 5. DEPENDENCY INVERSION PRINCIPLE (DIP)
# =======================

# BAD: High-level depends on low-level
class MySQLDatabaseBad:
    def connect(self):
        print("Connecting to MySQL")

class ApplicationBad:
    def __init__(self):
        self.db = MySQLDatabaseBad()

    def start(self):
        self.db.connect()


# GOOD: Depend on abstraction
class Database(ABC):
    @abstractmethod
    def connect(self): pass

class MySQL(Database):
    def connect(self): print("Connecting to MySQL")

class PostgreSQL(Database):
    def connect(self): print("Connecting to PostgreSQL")

class Application:
    def __init__(self, db: Database):
        self.db = db

    def start(self):
        self.db.connect()


# =======================
# MAIN (TESTING)
# =======================
if __name__ == "__main__":
    # SRP
    user = User("Pradeep")
    UserRepository().save(user)
    EmailService().send_email(user)

    # OCP
    calc = DiscountCalculator()
    print(calc.calculate(PremiumCustomerDiscount()))

    # LSP
    bird = Sparrow()
    bird.fly()

    # ISP
    robot = Robot()
    robot.work()

    # DIP
    app = Application(MySQL())
    app.start()
