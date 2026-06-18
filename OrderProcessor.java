import java.util.*;

// ❌ PROBLEMATIC CODE — DO NOT USE AS-IS
// Your task: identify SOLID violations and refactor

public class OrderProcessor {

    // Handles EVERYTHING — order logic, email, DB, discount, invoice
    public void processOrder(String customerType, List<String> items, String paymentMethod) {

        // 1. Calculate total
        double total = 0;
        for (String item : items) {
            if (item.equals("LAPTOP")) total += 80000;
            else if (item.equals("PHONE")) total += 30000;
            else if (item.equals("HEADPHONES")) total += 5000;
        }

        // 2. Apply discount — hardcoded logic
        if (customerType.equals("PREMIUM")) {
            total = total * 0.80; // 20% off
        } else if (customerType.equals("REGULAR")) {
            total = total * 0.95; // 5% off
        }
        // New customer type? You MUST modify this class ❌

        // 3. Process payment — hardcoded payment methods
        if (paymentMethod.equals("CREDIT_CARD")) {
            System.out.println("Processing credit card payment of ₹" + total);
            // CreditCard API logic here
        } else if (paymentMethod.equals("UPI")) {
            System.out.println("Processing UPI payment of ₹" + total);
            // UPI API logic here
        } else if (paymentMethod.equals("NETBANKING")) {
            System.out.println("Processing NetBanking payment of ₹" + total);
            // NetBanking API logic here
        }
        // New payment method? You MUST modify this class ❌

        // 4. Save to DB
        System.out.println("Saving order to database...");
        // DB logic directly in business class ❌

        // 5. Send email
        System.out.println("Sending confirmation email to customer...");
        // Email logic directly in business class ❌

        // 6. Generate Invoice — another responsibility crammed in
        System.out.println("=== INVOICE ===");
        System.out.println("Items: " + items);
        System.out.println("Total after discount: ₹" + total);
        System.out.println("Payment via: " + paymentMethod);
        // Invoice logic directly in business class ❌
    }
}


// ❌ Violates Liskov Substitution Principle
class Animal {
    public String sound() {
        return "Some sound";
    }
    public String fly() {
        return "I can fly!";
    }
}

class Dog extends Animal {
    @Override
    public String fly() {
        throw new UnsupportedOperationException("Dogs can't fly!"); // ❌ LSP violation
    }
}

class Bird extends Animal {
    @Override
    public String fly() {
        return "I am flying!";
    }
}


// ❌ Violates Interface Segregation Principle
interface WorkerInterface {
    void work();
    void eat();
    void sleep();
    void attendMeeting();
    void writeCode();
}

interface IHumanWorkerCapabilities{
    void work();
    void eat();
    void sleep();
    void attendMeeting();
    void writeCode();
}
interface IRobotWorkerCapabilities{
    void work();
    void attendMeeting();
    void writeCode();
}



class HumanWorker implements WorkerInterface {
    public void work() { System.out.println("Human working"); }
    public void eat() { System.out.println("Human eating"); }
    public void sleep() { System.out.println("Human sleeping"); }
    public void attendMeeting() { System.out.println("Human in meeting"); }
    public void writeCode() { System.out.println("Human writing code"); }
}

class RobotWorker implements WorkerInterface {
    public void work() { System.out.println("Robot working"); }
    public void eat() { throw new UnsupportedOperationException("Robots don't eat!"); } // ❌ ISP violation
    public void sleep() { throw new UnsupportedOperationException("Robots don't sleep!"); } // ❌ ISP violation
    public void attendMeeting() { System.out.println("Robot in meeting"); }
    public void writeCode() { System.out.println("Robot writing code"); }
}


// ❌ Violates Dependency Inversion Principle
class MySQLDatabase {
    public void save(String data) {
        System.out.println("Saving to MySQL: " + data);
    }
}

interface IDatabase{
    public void save(String data);
}


class MySQL implements IDatabase{

}

class No_Sql implements IDatabase{

}

class ReportService {
    private IDatabase database; //

    public ReportService(IDatabase database) {
        this.database = database;
    }

    public void setDataBase(IDatabase database){
        this.database = database;
    }

    public void generateReport(String data) {
        System.out.println("Generating report...");
        database.save(data);
    }
}

class ReportService {
    private MySQLDatabase database; // ❌ High-level module depends on low-level module directly

    public ReportService() {
        this.database = new MySQLDatabase(); // ❌ hardcoded dependency
    }

    public void generateReport(String data) {
        System.out.println("Generating report...");
        database.save(data);
    }
}