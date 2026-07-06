package DesignPattern;


//ATM Machine

// ATM States 
// no_card_state
// card_inserted_state

interface IATM_Machine_States{
    void insertCard();
    void ejectCard();
    void withDrawCash();
}

class ATM_Machine{
    IATM_Machine_States currentState;
    IATM_Machine_States no_card_state;
    IATM_Machine_States card_inserted_state;

    public ATM_Machine(){
        this.no_card_state = new No_Card_State(this);
        this.card_inserted_state = new Card_Inserted_State(this);
        this.currentState = no_card_state;
    }
    //other methods ..
    IATM_Machine_States get_no_card_state(){
        return no_card_state;
    }

    IATM_Machine_States get_card_inserted_state(){
        return card_inserted_state;
    }

    void setCurrentState(IATM_Machine_States state){
        this.currentState = state;
    }

    void insertCard(){
        currentState.insertCard();
    }
    void ejectCard() {
        currentState.ejectCard();
    }
    void withDrawCash() {
        currentState.insertCard();
    }
}

class No_Card_State implements IATM_Machine_States{

    ATM_Machine atm;
    public No_Card_State(ATM_Machine atm){
        this.atm = atm;
    }
    @Override
    public void insertCard() {
        System.out.println("Card is Inserted");
        atm.setCurrentState(atm.get_card_inserted_state());
    }

    @Override
    public void ejectCard() {
        System.out.println("No Card is present please insert card");
    }

    @Override
    public void withDrawCash() {
        System.out.println("No Card No Cash");
    }
}

class Card_Inserted_State implements IATM_Machine_States{
    ATM_Machine atm;
    public Card_Inserted_State(ATM_Machine atm){
        this.atm = atm;
    }
    @Override
    public void insertCard() {
        System.out.println("card is already inserted");
    }

    @Override
    public void ejectCard() {
        System.out.println("Card is ejected");
        atm.setCurrentState(atm.get_no_card_state());
    }

    @Override
    public void withDrawCash() {

    }
}


public class ATM_Machine_Demo {
    
}
