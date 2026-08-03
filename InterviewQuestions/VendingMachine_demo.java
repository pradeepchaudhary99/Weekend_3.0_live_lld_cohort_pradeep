/*
Vending Machine

Functional Requirements:

1. User should be able to insert money
2. User should be able to select the product
3. Machine should dispatch the selected Item after checking the valid transaction
4. User is exposed with multiple buttons/interface to interact with the machine
5. Machine should handle states carefully?
6. Machine should support multiple types of Coins

---- States of the Vending Machine ---------
IdleState HasMoneyState DispensingState ......

State Methods:
    +insertCoins()
    +SelectProduct
    +cancel()
    +dispense()


Non-Functional Requirements:

1. Coorrectness under concurrency
2. Extensibility




Core Entities:
Product
Slot
Inventory
paymentMethods

vendingState(Interface):
    +insertCoins()
    +SelectProduct
    +cancel()
    +dispense()

VendingMachineStates:
1. IdleState
2. HashMoneyState
3. DispensingState

*/

import java.util.LinkedHashMap;
import java.util.Map;

class Product {
    final String code;
    final String name;
    final double price;
    int quantity;

    Product(String code, String name, double price, int quantity) {
        this.code = code;
        this.name = name;
        this.price = price;
        this.quantity = quantity;
    }
}

interface VendingMachineStates {
    void insertCoins(double amount);
    void selectProduct(String code);
    void cancel();
    void dispense();
}

class VendingMachine {
    final Map<String, Product> inventory = new LinkedHashMap<>();
    double balance;
    Product selectedProduct;

    final VendingMachineStates idleState = new IdleState(this);
    final VendingMachineStates hasMoneyState = new HasMoneyState(this);
    final VendingMachineStates dispensingState = new DispensingState(this);
    VendingMachineStates currentState = idleState;

    void addProduct(Product product) {
        inventory.put(product.code, product);
    }

    void setState(VendingMachineStates state) {
        this.currentState = state;
    }

    void addBalance(double amount) {
        balance += amount;
    }

    void insertCoins(double amount) {
        currentState.insertCoins(amount);
    }

    void selectProduct(String code) {
        currentState.selectProduct(code);
    }

    void cancel() {
        currentState.cancel();
    }

    void dispense() {
        currentState.dispense();
    }

    void reset() {
        balance = 0;
        selectedProduct = null;
        currentState = idleState;
    }
}

class IdleState implements VendingMachineStates {
    private final VendingMachine machine;

    IdleState(VendingMachine machine) {
        this.machine = machine;
    }

    @Override
    public void insertCoins(double amount) {
        machine.addBalance(amount);
        machine.setState(machine.hasMoneyState);
        System.out.printf("Inserted %.2f. Current balance: %.2f%n", amount, machine.balance);
    }

    @Override
    public void selectProduct(String code) {
        System.out.println("Insert coins first");
    }

    @Override
    public void cancel() {
        System.out.println("Nothing to cancel");
    }

    @Override
    public void dispense() {
        System.out.println("Select a product first");
    }
}

class HasMoneyState implements VendingMachineStates {
    private final VendingMachine machine;

    HasMoneyState(VendingMachine machine) {
        this.machine = machine;
    }

    @Override
    public void insertCoins(double amount) {
        machine.addBalance(amount);
        System.out.printf("Inserted %.2f. Current balance: %.2f%n", amount, machine.balance);
    }

    @Override
    public void selectProduct(String code) {
        Product product = machine.inventory.get(code);
        if (product == null || product.quantity <= 0) {
            System.out.println("Product " + code + " is unavailable");
            return;
        }
        if (machine.balance < product.price) {
            System.out.printf("Insufficient balance for %s: need %.2f, have %.2f%n",
                    product.name, product.price, machine.balance);
            return;
        }
        machine.selectedProduct = product;
        machine.setState(machine.dispensingState);
        machine.dispense();
    }

    @Override
    public void cancel() {
        System.out.printf("Refunding %.2f%n", machine.balance);
        machine.reset();
    }

    @Override
    public void dispense() {
        System.out.println("Select a product first");
    }
}

class DispensingState implements VendingMachineStates {
    private final VendingMachine machine;

    DispensingState(VendingMachine machine) {
        this.machine = machine;
    }

    @Override
    public void insertCoins(double amount) {
        System.out.println("Already dispensing, please wait");
    }

    @Override
    public void selectProduct(String code) {
        System.out.println("Already dispensing, please wait");
    }

    @Override
    public void cancel() {
        System.out.println("Cannot cancel while dispensing");
    }

    @Override
    public void dispense() {
        Product product = machine.selectedProduct;
        product.quantity--;
        double change = machine.balance - product.price;
        System.out.printf("Dispensing %s. Change returned: %.2f%n", product.name, change);
        machine.reset();
    }
}

public class VendingMachine_demo {
    public static void main(String[] args) {
        VendingMachine machine = new VendingMachine();
        machine.addProduct(new Product("A1", "Coke", 1.50, 2));
        machine.addProduct(new Product("A2", "Chips", 2.00, 1));

        System.out.println("-- Trying to select without inserting money --");
        machine.selectProduct("A1");

        System.out.println("\n-- Insert 1.00, try to buy 1.50 Coke (insufficient) --");
        machine.insertCoins(1.00);
        machine.selectProduct("A1");

        System.out.println("\n-- Insert another 1.00 (total 2.00), buy Coke --");
        machine.insertCoins(1.00);
        machine.selectProduct("A1");

        System.out.println("\n-- Insert 2.00, then cancel --");
        machine.insertCoins(2.00);
        machine.cancel();

        System.out.println("\n-- Buy the last Coke, then try to buy Coke again --");
        machine.insertCoins(1.50);
        machine.selectProduct("A1");
        machine.insertCoins(1.50);
        machine.selectProduct("A1");
    }
}
