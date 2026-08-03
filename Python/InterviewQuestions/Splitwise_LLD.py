"""
SplitWise System / App
------------------------------

Functional Requirements:
    Create Users
    Create Groups
    Add Expenses in the system
    Split Types
        - Equal Split
        - Percentage Split
    Maintain balances
        balances[A][B] --> what B owes A in A's balance sheet
    Show balances of a User
    should support payments

Non-Functional:
    Thread-Safety
    Consistency across the balance sheet (ACID Compliance)
    Extensibility to support multiple types of splits/expense

----- Out of Scope -------
    Simplification feature of splitwise: Graph Algorithm using Priority Queue
    Settle

Entities:
    User
    Group
    Expense
    SplitStrategy (Interface)
    EqualSplit
    PercentageSplit
    SplitWiseService
    BalanceManagerService
"""

import threading
from abc import ABC, abstractmethod
from typing import Dict, List


class User:
    def __init__(self, user_id: str, name: str):
        self._id = user_id
        self._name = name

    def get_id(self) -> str:
        return self._id

    def get_name(self) -> str:
        return self._name

    def __repr__(self) -> str:
        return f"User({self._id}, {self._name})"


class Split:
    def __init__(self, paid_by: User, paid_for: User, split_amount: float, expense_id: str):
        self.paid_by = paid_by
        self.paid_for = paid_for
        self.split_amount = split_amount
        self.expense_id = expense_id


class Expense:
    def __init__(self, expense_id: str, paid_by: User, amount: float,
                 users_participated: List[User], strategy: "SplitStrategy"):
        self.id = expense_id
        self.paid_by = paid_by
        self.amount = amount
        self.users_participated = users_participated
        self.strategy = strategy
        self.splits: List[Split] = []

    def calculate_share(self) -> List[Split]:
        self.splits = self.strategy.generate_split(self)
        return self.splits


class SplitStrategy(ABC):
    @abstractmethod
    def generate_split(self, expense: Expense) -> List[Split]:
        raise NotImplementedError


class EqualSplit(SplitStrategy):
    """Splits the amount equally among every participant except the payer."""

    def generate_split(self, expense: Expense) -> List[Split]:
        payers = [u for u in expense.users_participated if u.get_id() != expense.paid_by.get_id()]
        if not payers:
            return []
        share = expense.amount / len(expense.users_participated)
        return [Split(expense.paid_by, user, share, expense.id) for user in payers]


class PercentageSplit(SplitStrategy):
    """Splits the amount according to a percentage assigned to each participant."""

    def __init__(self, percentages: Dict[str, float]):
        # user_id -> percentage of the total amount (0-100)
        self._percentages = percentages

    def generate_split(self, expense: Expense) -> List[Split]:
        splits = []
        for user in expense.users_participated:
            if user.get_id() == expense.paid_by.get_id():
                continue
            percentage = self._percentages.get(user.get_id(), 0.0)
            splits.append(Split(expense.paid_by, user, expense.amount * percentage / 100.0, expense.id))
        return splits


class Group:
    def __init__(self, group_id: str, name: str):
        self.id = group_id
        self.name = name
        self.users: List[User] = []
        self.expenses: List[Expense] = []


class BalanceManagerService:
    """balances[a][b] is what user b owes user a."""

    def __init__(self):
        self._balances: Dict[str, Dict[str, float]] = {}
        self._lock = threading.Lock()

    def _ensure_user(self, user_id: str) -> None:
        self._balances.setdefault(user_id, {})

    def update_balance_sheet(self, expense: Expense, splits: List[Split]) -> None:
        with self._lock:
            for split in splits:
                payer_id = split.paid_by.get_id()
                debtor_id = split.paid_for.get_id()
                self._ensure_user(payer_id)
                self._ensure_user(debtor_id)

                self._balances[payer_id][debtor_id] = (
                    self._balances[payer_id].get(debtor_id, 0.0) + split.split_amount
                )
                self._balances[debtor_id][payer_id] = (
                    self._balances[debtor_id].get(payer_id, 0.0) - split.split_amount
                )

    def get_balance(self, user_id: str) -> Dict[str, float]:
        with self._lock:
            return dict(self._balances.get(user_id, {}))


class SplitWiseService:
    def __init__(self):
        self._users: Dict[str, User] = {}
        self._groups: Dict[str, Group] = {}
        self._balance_manager_service = BalanceManagerService()

    def create_user(self, user_id: str, name: str) -> User:
        user = User(user_id, name)
        self._users[user_id] = user
        return user

    def create_group(self, group_id: str, name: str, users: List[User]) -> Group:
        group = Group(group_id, name)
        group.users = users
        self._groups[group_id] = group
        return group

    def add_expense(self, expense_id: str, paid_by: User, amount: float,
                     users_participated: List[User], strategy: SplitStrategy) -> Expense:
        expense = Expense(expense_id, paid_by, amount, users_participated, strategy)
        splits = expense.calculate_share()
        self._balance_manager_service.update_balance_sheet(expense, splits)
        return expense

    def show_balance(self, user: User) -> None:
        balances = self._balance_manager_service.get_balance(user.get_id())
        if not balances or all(amount == 0 for amount in balances.values()):
            print(f"No pending balances for {user.get_name()}")
            return
        for other_id, amount in balances.items():
            if amount > 0:
                print(f"{other_id} owes {user.get_name()} {amount:.2f}")
            elif amount < 0:
                print(f"{user.get_name()} owes {other_id} {-amount:.2f}")


def main() -> None:
    service = SplitWiseService()

    alice = service.create_user("u1", "Alice")
    bob = service.create_user("u2", "Bob")
    carol = service.create_user("u3", "Carol")

    service.create_group("g1", "Goa Trip", [alice, bob, carol])

    print("Equal split: Alice pays 300 for all three")
    service.add_expense("e1", alice, 300.0, [alice, bob, carol], EqualSplit())

    print("Percentage split: Bob pays 200, Alice 30%, Carol 70%")
    service.add_expense(
        "e2", bob, 200.0, [alice, bob, carol],
        PercentageSplit({"u1": 30.0, "u3": 70.0}),
    )

    for user in (alice, bob, carol):
        print(f"\nBalances for {user.get_name()}:")
        service.show_balance(user)


if __name__ == "__main__":
    main()
