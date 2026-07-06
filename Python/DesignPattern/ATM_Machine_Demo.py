"""ATM Machine - State Design Pattern"""

from abc import ABC, abstractmethod


class IATMMachineStates(ABC):
    @abstractmethod
    def insert_card(self):
        pass

    @abstractmethod
    def eject_card(self):
        pass

    @abstractmethod
    def with_draw_cash(self):
        pass


class ATMMachine:
    def __init__(self):
        self.no_card_state = NoCardState(self)
        self.card_inserted_state = CardInsertedState(self)
        self.current_state = self.no_card_state

    def get_no_card_state(self):
        return self.no_card_state

    def get_card_inserted_state(self):
        return self.card_inserted_state

    def set_current_state(self, state: IATMMachineStates):
        self.current_state = state

    def insert_card(self):
        self.current_state.insert_card()

    def eject_card(self):
        self.current_state.eject_card()

    def with_draw_cash(self):
        self.current_state.insert_card()


class NoCardState(IATMMachineStates):
    def __init__(self, atm: ATMMachine):
        self.atm = atm

    def insert_card(self):
        print("Card is Inserted")
        self.atm.set_current_state(self.atm.get_card_inserted_state())

    def eject_card(self):
        print("No Card is present please insert card")

    def with_draw_cash(self):
        print("No Card No Cash")


class CardInsertedState(IATMMachineStates):
    def __init__(self, atm: ATMMachine):
        self.atm = atm

    def insert_card(self):
        print("card is already inserted")

    def eject_card(self):
        print("Card is ejected")
        self.atm.set_current_state(self.atm.get_no_card_state())

    def with_draw_cash(self):
        pass


if __name__ == "__main__":
    atm = ATMMachine()
    atm.insert_card()
    atm.eject_card()
