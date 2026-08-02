// Vending Machine
//
// Functional Requirements:
//
// 1. User should be able to insert money
// 2. User should be able to select the product
// 3. Machine should dispatch the selected Item after checking the valid transaction
// 4. User is exposed with multiple buttons/interface to interact with the machine
// 5. Machine should handle states carefully?
// 6. Machine should support multiple types of Coins
//
// ---- States of the Vending Machine ---------
// IdleState HasMoneyState DispensingState ......
//
// State Methods:
//     +insertCoins()
//     +SelectProduct
//     +cancel()
//     +dispense()
//
//
// Non-Functional Requirements:
//
// 1. Coorrectness under concurrency
// 2. Extensibility
//
//
// Core Entities:
// Product
// Slot
// Inventory
// paymentMethods
//
// vendingState(Interface):
//     +insertCoins()
//     +SelectProduct
//     +cancel()
//     +dispense()
//
// VendingMachineStates:
// 1. IdleState
// 2. HashMoneyState
// 3. DispensingState

#include <memory>
#include <stdexcept>

class Product {};

class Slot {};

struct VendingMachineStates {
    virtual void insertCoins() = 0;
    virtual void selectProducts() = 0;
    virtual void cancel() = 0;
    virtual void dispense() = 0;
    virtual ~VendingMachineStates() = default;
};

class VendingMachine {
public:
    std::shared_ptr<VendingMachineStates> currentState;
    std::shared_ptr<VendingMachineStates> idleState;
    std::shared_ptr<VendingMachineStates> hasMoneyMachineStates;
    std::shared_ptr<VendingMachineStates> dispMachineStates;

    std::shared_ptr<VendingMachineStates> gethasMoneyState() { return hasMoneyMachineStates; }

    void setState(std::shared_ptr<VendingMachineStates> state) { currentState = state; }

    void insertCoins() { currentState->insertCoins(); }

    void selectProducts() { currentState->selectProducts(); }

    void cancel() {}

    void dispense() {}
};

class IdleState : public VendingMachineStates {
    VendingMachine* machine;

public:
    explicit IdleState(VendingMachine* machine) : machine(machine) {}

    void insertCoins() override { machine->setState(machine->gethasMoneyState()); }

    void selectProducts() override {
        throw std::runtime_error("Unimplemented method 'selectProducts'");
    }

    void cancel() override {
        throw std::runtime_error("Unimplemented method 'cancel'");
    }

    void dispense() override {
        throw std::runtime_error("Unimplemented method 'dispense'");
    }
};

class HasMoneyState : public VendingMachineStates {
public:
    void insertCoins() override {
        throw std::runtime_error("Unimplemented method 'insertCoins'");
    }

    void selectProducts() override {
        throw std::runtime_error("Unimplemented method 'selectProducts'");
    }

    void cancel() override {
        throw std::runtime_error("Unimplemented method 'cancel'");
    }

    void dispense() override {
        throw std::runtime_error("Unimplemented method 'dispense'");
    }
};

class DispensingState : public VendingMachineStates {
public:
    void insertCoins() override {
        throw std::runtime_error("Unimplemented method 'insertCoins'");
    }

    void selectProducts() override {
        throw std::runtime_error("Unimplemented method 'selectProducts'");
    }

    void cancel() override {
        throw std::runtime_error("Unimplemented method 'cancel'");
    }

    void dispense() override {
        throw std::runtime_error("Unimplemented method 'dispense'");
    }
};

class VendingMachineDemo {};
