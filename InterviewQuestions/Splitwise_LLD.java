

/*
SplitWise System /App
Functional Requirements:
    Create Users 
    Create Groups
    Add Expenses in the system 
    Split Types
        - Equal Split 
        - Exact Split 
        - Share Split
    Maintain balances 
        Balance[A][B] --> status of B in A's Balance sheet 
    Show balances of a User 

    should  support payments
Non-Functional 
1. Thread-Safety 
2. Consistency across the balance sheet (ACID Compliance)
3. Extensibility to support multiple types of splits/expense  


----- OUT of Scope -------
    Simplification feature of splitwise : Graph Algorithm using Priority Queue
    Settle

Core Entities: 

User 
Groups 
Expense 
SplitStrategy(Interface)
EqualSplit
ExactSplit
PercentageSplit 

SplitWiseService

BalanceManagerService 

*/

class User{
    String name;
    String id;
    getId();
    getName();
}

class Group{
    id;
    name;
    List<User> users;   // 10,   
    List<Expense> expenses;
}

class Expense{
    id;
    User paidBy;
    double amount;
    List<User> users_participated;
    SplitStrategy strategy;
    List<Split> splits;
    calculateShare();
}

class Split{
    User paidBy;
    User paidFor;
    double splitAmount;
    String expenseId;
}

interface SplitStrategy{
    List<Split> generateSplit(Expense expense);
}

class EqualSplit implements SplitStrategy{

}

class PercentageSplit implements SplitStrategy{
    
}

class SplitWiseService{
    Map<String, User> usersRepository;  //UserId, User 
    BalanceManagerService balanceManagerService;
    Map<String, Group> groupRepository; // GroupId, Group 

    createUser();
    createGroup();
    addExpense();
    showBalance(User);

}



class BalanceManagerService{
    Map<User, Map<User, Double>> balances; 

    updateBalanceSheet(Expense expense, List<Split> splits){

        for(Split split : splits){
            balances.get(split.paidBy).put(split.paidFor,  balances.get(split.paidBy).get(split.paidFor) + split.amount);
            balances.get(split.paidFor).put(split.paidBy,  balances.get(split.paidFor).get(split.paidBy) - split.amount);
            
        }
    }
    getBalance(User userId);
}





public class Splitwise_LLD {
    
}
