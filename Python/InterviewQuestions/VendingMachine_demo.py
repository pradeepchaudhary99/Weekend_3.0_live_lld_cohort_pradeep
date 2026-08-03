"""
Vending Machine
------------------------------

Functional Requirements:

1. User should be able to insert money
2. User should be able to select the product
3. Machine should dispatch the selected item after checking the valid transaction
4. User is exposed with multiple buttons/interface to interact with the machine
5. Machine should handle states carefully
6. Machine should support multiple types of coins

---- States of the Vending Machine ---------
IdleState, HasMoneyState, DispensingState

State Methods:
    insert_coins()
    select_product()
    cancel()
    dispense()

Non-Functional Requirements:

1. Correctness under concurrency
2. Extensibility

Entities:
    Product
    VendingMachineStates (interface): IdleState, HasMoneyState, DispensingState
    VendingMachine
"""

from abc import ABC, abstractmethod
from typing import Dict, Optional


class Product:
    def __init__(self, code: str, name: str, price: float, quantity: int):
        self.code = code
        self.name = name
        self.price = price
        self.quantity = quantity


class VendingMachineStates(ABC):
    @abstractmethod
    def insert_coins(self, amount: float) -> None:
        pass

    @abstractmethod
    def select_product(self, code: str) -> None:
        pass

    @abstractmethod
    def cancel(self) -> None:
        pass

    @abstractmethod
    def dispense(self) -> None:
        pass


class VendingMachine:
    def __init__(self):
        self.inventory: Dict[str, Product] = {}
        self.balance: float = 0.0
        self.selected_product: Optional[Product] = None

        self.idle_state: VendingMachineStates = IdleState(self)
        self.has_money_state: VendingMachineStates = HasMoneyState(self)
        self.dispensing_state: VendingMachineStates = DispensingState(self)
        self.current_state: VendingMachineStates = self.idle_state

    def add_product(self, product: Product) -> None:
        self.inventory[product.code] = product

    def set_state(self, state: VendingMachineStates) -> None:
        self.current_state = state

    def add_balance(self, amount: float) -> None:
        self.balance += amount

    def insert_coins(self, amount: float) -> None:
        self.current_state.insert_coins(amount)

    def select_product(self, code: str) -> None:
        self.current_state.select_product(code)

    def cancel(self) -> None:
        self.current_state.cancel()

    def dispense(self) -> None:
        self.current_state.dispense()

    def reset(self) -> None:
        self.balance = 0.0
        self.selected_product = None
        self.current_state = self.idle_state


class IdleState(VendingMachineStates):
    def __init__(self, machine: VendingMachine):
        self.machine = machine

    def insert_coins(self, amount: float) -> None:
        self.machine.add_balance(amount)
        self.machine.set_state(self.machine.has_money_state)
        print(f"Inserted {amount:.2f}. Current balance: {self.machine.balance:.2f}")

    def select_product(self, code: str) -> None:
        print("Insert coins first")

    def cancel(self) -> None:
        print("Nothing to cancel")

    def dispense(self) -> None:
        print("Select a product first")


class HasMoneyState(VendingMachineStates):
    def __init__(self, machine: VendingMachine):
        self.machine = machine

    def insert_coins(self, amount: float) -> None:
        self.machine.add_balance(amount)
        print(f"Inserted {amount:.2f}. Current balance: {self.machine.balance:.2f}")

    def select_product(self, code: str) -> None:
        product = self.machine.inventory.get(code)
        if product is None or product.quantity <= 0:
            print(f"Product {code} is unavailable")
            return
        if self.machine.balance < product.price:
            print(f"Insufficient balance for {product.name}: "
                  f"need {product.price:.2f}, have {self.machine.balance:.2f}")
            return
        self.machine.selected_product = product
        self.machine.set_state(self.machine.dispensing_state)
        self.machine.dispense()

    def cancel(self) -> None:
        print(f"Refunding {self.machine.balance:.2f}")
        self.machine.reset()

    def dispense(self) -> None:
        print("Select a product first")


class DispensingState(VendingMachineStates):
    def __init__(self, machine: VendingMachine):
        self.machine = machine

    def insert_coins(self, amount: float) -> None:
        print("Already dispensing, please wait")

    def select_product(self, code: str) -> None:
        print("Already dispensing, please wait")

    def cancel(self) -> None:
        print("Cannot cancel while dispensing")

    def dispense(self) -> None:
        product = self.machine.selected_product
        product.quantity -= 1
        change = self.machine.balance - product.price
        print(f"Dispensing {product.name}. Change returned: {change:.2f}")
        self.machine.reset()


def main() -> None:
    machine = VendingMachine()
    machine.add_product(Product("A1", "Coke", 1.50, 2))
    machine.add_product(Product("A2", "Chips", 2.00, 1))

    print("-- Trying to select without inserting money --")
    machine.select_product("A1")

    print("\n-- Insert 1.00, try to buy 1.50 Coke (insufficient) --")
    machine.insert_coins(1.00)
    machine.select_product("A1")

    print("\n-- Insert another 1.00 (total 2.00), buy Coke --")
    machine.insert_coins(1.00)
    machine.select_product("A1")

    print("\n-- Insert 2.00, then cancel --")
    machine.insert_coins(2.00)
    machine.cancel()

    print("\n-- Buy the last Coke, then try to buy Coke again --")
    machine.insert_coins(1.50)
    machine.select_product("A1")
    machine.insert_coins(1.50)
    machine.select_product("A1")


if __name__ == "__main__":
    main()
