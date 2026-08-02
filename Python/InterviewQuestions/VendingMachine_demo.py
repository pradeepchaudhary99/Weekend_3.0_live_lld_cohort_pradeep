# Vending Machine
#
# Functional Requirements:
#
# 1. User should be able to insert money
# 2. User should be able to select the product
# 3. Machine should dispatch the selected Item after checking the valid transaction
# 4. User is exposed with multiple buttons/interface to interact with the machine
# 5. Machine should handle states carefully?
# 6. Machine should support multiple types of Coins
#
# ---- States of the Vending Machine ---------
# IdleState HasMoneyState DispensingState ......
#
# State Methods:
#     +insertCoins()
#     +SelectProduct
#     +cancel()
#     +dispense()
#
#
# Non-Functional Requirements:
#
# 1. Coorrectness under concurrency
# 2. Extensibility
#
#
# Core Entities:
# Product
# Slot
# Inventory
# paymentMethods
#
# vendingState(Interface):
#     +insertCoins()
#     +SelectProduct
#     +cancel()
#     +dispense()
#
# VendingMachineStates:
# 1. IdleState
# 2. HashMoneyState
# 3. DispensingState

from abc import ABC, abstractmethod
from typing import Optional


class Product:
    pass


class Slot:
    pass


class VendingMachineStates(ABC):
    @abstractmethod
    def insert_coins(self):
        pass

    @abstractmethod
    def select_products(self):
        pass

    @abstractmethod
    def cancel(self):
        pass

    @abstractmethod
    def dispense(self):
        pass


class VendingMachine:
    def __init__(self):
        self.current_state: Optional[VendingMachineStates] = None
        self.idle_state: Optional[VendingMachineStates] = None
        self.has_money_machine_states: Optional[VendingMachineStates] = None
        self.disp_machine_states: Optional[VendingMachineStates] = None

    def get_has_money_state(self) -> Optional[VendingMachineStates]:
        return self.has_money_machine_states

    def set_state(self, state: VendingMachineStates):
        self.current_state = state

    def insert_coins(self):
        self.current_state.insert_coins()

    def select_products(self):
        self.current_state.select_products()

    def cancel(self):
        pass

    def dispense(self):
        pass


class IdleState(VendingMachineStates):
    def __init__(self, machine: VendingMachine):
        self.machine = machine

    def insert_coins(self):
        self.machine.set_state(self.machine.get_has_money_state())

    def select_products(self):
        raise NotImplementedError("Unimplemented method 'select_products'")

    def cancel(self):
        raise NotImplementedError("Unimplemented method 'cancel'")

    def dispense(self):
        raise NotImplementedError("Unimplemented method 'dispense'")


class HasMoneyState(VendingMachineStates):
    def insert_coins(self):
        raise NotImplementedError("Unimplemented method 'insert_coins'")

    def select_products(self):
        raise NotImplementedError("Unimplemented method 'select_products'")

    def cancel(self):
        raise NotImplementedError("Unimplemented method 'cancel'")

    def dispense(self):
        raise NotImplementedError("Unimplemented method 'dispense'")


class DispensingState(VendingMachineStates):
    def insert_coins(self):
        raise NotImplementedError("Unimplemented method 'insert_coins'")

    def select_products(self):
        raise NotImplementedError("Unimplemented method 'select_products'")

    def cancel(self):
        raise NotImplementedError("Unimplemented method 'cancel'")

    def dispense(self):
        raise NotImplementedError("Unimplemented method 'dispense'")


class VendingMachineDemo:
    pass
