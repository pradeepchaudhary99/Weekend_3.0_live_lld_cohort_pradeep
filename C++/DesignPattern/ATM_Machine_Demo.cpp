#include <iostream>
#include <memory>
using namespace std;

// ATM Machine - State Design Pattern

class ATMMachine;

struct IATMMachineStates {
    virtual void insertCard() = 0;
    virtual void ejectCard() = 0;
    virtual void withDrawCash() = 0;
    virtual ~IATMMachineStates() = default;
};

class ATMMachine {
    shared_ptr<IATMMachineStates> currentState;
    shared_ptr<IATMMachineStates> noCardState;
    shared_ptr<IATMMachineStates> cardInsertedState;

public:
    ATMMachine();

    shared_ptr<IATMMachineStates> getNoCardState() { return noCardState; }
    shared_ptr<IATMMachineStates> getCardInsertedState() { return cardInsertedState; }

    void setCurrentState(shared_ptr<IATMMachineStates> state) { currentState = move(state); }

    void insertCard();
    void ejectCard();
    void withDrawCash();
};

class NoCardState : public IATMMachineStates {
    ATMMachine* atm;

public:
    explicit NoCardState(ATMMachine* atm) : atm(atm) {}

    void insertCard() override {
        cout << "Card is Inserted\n";
        atm->setCurrentState(atm->getCardInsertedState());
    }

    void ejectCard() override {
        cout << "No Card is present please insert card\n";
    }

    void withDrawCash() override {
        cout << "No Card No Cash\n";
    }
};

class CardInsertedState : public IATMMachineStates {
    ATMMachine* atm;

public:
    explicit CardInsertedState(ATMMachine* atm) : atm(atm) {}

    void insertCard() override {
        cout << "card is already inserted\n";
    }

    void ejectCard() override {
        cout << "Card is ejected\n";
        atm->setCurrentState(atm->getNoCardState());
    }

    void withDrawCash() override {}
};

ATMMachine::ATMMachine() {
    noCardState = make_shared<NoCardState>(this);
    cardInsertedState = make_shared<CardInsertedState>(this);
    currentState = noCardState;
}

void ATMMachine::insertCard() { currentState->insertCard(); }
void ATMMachine::ejectCard() { currentState->ejectCard(); }
void ATMMachine::withDrawCash() { currentState->insertCard(); }

int main() {
    ATMMachine atm;
    atm.insertCard();
    atm.ejectCard();
    return 0;
}
