/*
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
    insertCoins()
    selectProduct()
    cancel()
    dispense()

Non-Functional Requirements:

1. Correctness under concurrency
2. Extensibility

Entities:
    Product
    VendingMachineStates (interface): IdleState, HasMoneyState, DispensingState
    VendingMachine
*/

#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <string>

struct Product {
    std::string code;
    std::string name;
    double price;
    int quantity;

    Product(std::string code, std::string name, double price, int quantity)
        : code(std::move(code)), name(std::move(name)), price(price), quantity(quantity) {}
};

class VendingMachine;

struct VendingMachineStates {
    virtual void insertCoins(double amount) = 0;
    virtual void selectProduct(const std::string& code) = 0;
    virtual void cancel() = 0;
    virtual void dispense() = 0;
    virtual ~VendingMachineStates() = default;
};

class VendingMachine {
public:
    std::map<std::string, std::shared_ptr<Product>> inventory;
    double balance = 0.0;
    std::shared_ptr<Product> selectedProduct;

    std::shared_ptr<VendingMachineStates> idleState;
    std::shared_ptr<VendingMachineStates> hasMoneyState;
    std::shared_ptr<VendingMachineStates> dispensingState;
    std::shared_ptr<VendingMachineStates> currentState;

    void addProduct(const std::shared_ptr<Product>& product) { inventory[product->code] = product; }

    void setState(std::shared_ptr<VendingMachineStates> state) { currentState = std::move(state); }

    void addBalance(double amount) { balance += amount; }

    void insertCoins(double amount) { currentState->insertCoins(amount); }

    void selectProduct(const std::string& code) { currentState->selectProduct(code); }

    void cancel() { currentState->cancel(); }

    void dispense() { currentState->dispense(); }

    void reset() {
        balance = 0.0;
        selectedProduct = nullptr;
        currentState = idleState;
    }
};

class IdleState : public VendingMachineStates {
    VendingMachine* machine;

public:
    explicit IdleState(VendingMachine* machine) : machine(machine) {}

    void insertCoins(double amount) override {
        machine->addBalance(amount);
        machine->setState(machine->hasMoneyState);
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Inserted " << amount << ". Current balance: " << machine->balance << std::endl;
    }

    void selectProduct(const std::string& code) override { std::cout << "Insert coins first" << std::endl; }

    void cancel() override { std::cout << "Nothing to cancel" << std::endl; }

    void dispense() override { std::cout << "Select a product first" << std::endl; }
};

class HasMoneyState : public VendingMachineStates {
    VendingMachine* machine;

public:
    explicit HasMoneyState(VendingMachine* machine) : machine(machine) {}

    void insertCoins(double amount) override {
        machine->addBalance(amount);
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Inserted " << amount << ". Current balance: " << machine->balance << std::endl;
    }

    void selectProduct(const std::string& code) override {
        auto it = machine->inventory.find(code);
        if (it == machine->inventory.end() || it->second->quantity <= 0) {
            std::cout << "Product " << code << " is unavailable" << std::endl;
            return;
        }
        auto product = it->second;
        if (machine->balance < product->price) {
            std::cout << std::fixed << std::setprecision(2);
            std::cout << "Insufficient balance for " << product->name << ": need " << product->price
                       << ", have " << machine->balance << std::endl;
            return;
        }
        machine->selectedProduct = product;
        machine->setState(machine->dispensingState);
        machine->dispense();
    }

    void cancel() override {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Refunding " << machine->balance << std::endl;
        machine->reset();
    }

    void dispense() override { std::cout << "Select a product first" << std::endl; }
};

class DispensingState : public VendingMachineStates {
    VendingMachine* machine;

public:
    explicit DispensingState(VendingMachine* machine) : machine(machine) {}

    void insertCoins(double amount) override { std::cout << "Already dispensing, please wait" << std::endl; }

    void selectProduct(const std::string& code) override {
        std::cout << "Already dispensing, please wait" << std::endl;
    }

    void cancel() override { std::cout << "Cannot cancel while dispensing" << std::endl; }

    void dispense() override {
        auto product = machine->selectedProduct;
        product->quantity--;
        double change = machine->balance - product->price;
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Dispensing " << product->name << ". Change returned: " << change << std::endl;
        machine->reset();
    }
};

int main() {
    VendingMachine machine;
    machine.idleState = std::make_shared<IdleState>(&machine);
    machine.hasMoneyState = std::make_shared<HasMoneyState>(&machine);
    machine.dispensingState = std::make_shared<DispensingState>(&machine);
    machine.currentState = machine.idleState;

    machine.addProduct(std::make_shared<Product>("A1", "Coke", 1.50, 2));
    machine.addProduct(std::make_shared<Product>("A2", "Chips", 2.00, 1));

    std::cout << "-- Trying to select without inserting money --" << std::endl;
    machine.selectProduct("A1");

    std::cout << "\n-- Insert 1.00, try to buy 1.50 Coke (insufficient) --" << std::endl;
    machine.insertCoins(1.00);
    machine.selectProduct("A1");

    std::cout << "\n-- Insert another 1.00 (total 2.00), buy Coke --" << std::endl;
    machine.insertCoins(1.00);
    machine.selectProduct("A1");

    std::cout << "\n-- Insert 2.00, then cancel --" << std::endl;
    machine.insertCoins(2.00);
    machine.cancel();

    std::cout << "\n-- Buy the last Coke, then try to buy Coke again --" << std::endl;
    machine.insertCoins(1.50);
    machine.selectProduct("A1");
    machine.insertCoins(1.50);
    machine.selectProduct("A1");

    return 0;
}
