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

class Product{

}

class Slot{

}

class VendingMachine{
    VendingMachineStates currentState; 
    VendingMachineStates idleState;
    VendingMachineStates hasMoneyMachineStates;
    VendingMachineStates dispMachineStates;

    VendingMachineStates gethasMoneyState(){
        return hasMoneyMachineStates;
    }

    void setState(VendingMachineStates state){
        this.currentState = state;
    }
    void insertCoins(){
        currentState.insertCoins();
    }

    void selectProducts(){
        currentState.selectProducts();
    }

    void cancel(){

    }

    void dispense(){

    }
}


interface VendingMachineStates{
    void insertCoins();
    void selectProducts();
    void cancel();
    void dispense();
}

class IdleState implements VendingMachineStates{
    VendingMachine machine;
    public IdleState(VendingMachine machine){
        this.machine = machine;
    }
    @Override
    public void insertCoins() {
        machine.setState(machine.gethasMoneyState());
    }

    @Override
    public void selectProducts() {
        // TODO Auto-generated method stub
        throw new UnsupportedOperationException("Unimplemented method 'selectProducts'");
    }

    @Override
    public void cancel() {
        // TODO Auto-generated method stub
        throw new UnsupportedOperationException("Unimplemented method 'cancel'");
    }

    @Override
    public void dispense() {
        // TODO Auto-generated method stub
        throw new UnsupportedOperationException("Unimplemented method 'dispense'");
    }

}

class HasMoneyState implements{

}

class DispensingState implements{

}








public class VendingMachine_demo {
    
}
