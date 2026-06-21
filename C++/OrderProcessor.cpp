// OrderProcessor — SOLID violations demo
#include <iostream>
#include <string>
#include <vector>
using namespace std;

// =======================
// BAD: Violates SRP, OCP, DIP
// =======================
class OrderProcessor {
public:
    void processOrder(const string& customerType, const vector<string>& items, const string& paymentMethod) {

        // 1. Calculate total
        double total = 0;
        for (const auto& item : items) {
            if (item == "LAPTOP")      total += 80000;
            else if (item == "PHONE")  total += 30000;
            else if (item == "HEADPHONES") total += 5000;
        }

        // 2. Apply discount (OCP violation)
        if (customerType == "PREMIUM")     total *= 0.80;
        else if (customerType == "REGULAR") total *= 0.95;

        // 3. Process payment (OCP violation)
        if (paymentMethod == "CREDIT_CARD")
            cout << "Processing credit card payment of Rs." << total << "\n";
        else if (paymentMethod == "UPI")
            cout << "Processing UPI payment of Rs." << total << "\n";
        else if (paymentMethod == "NETBANKING")
            cout << "Processing NetBanking payment of Rs." << total << "\n";

        // 4. Save to DB (SRP violation)
        cout << "Saving order to database...\n";

        // 5. Send email (SRP violation)
        cout << "Sending confirmation email to customer...\n";

        // 6. Generate invoice (SRP violation)
        cout << "=== INVOICE ===\n";
        cout << "Total after discount: Rs." << total << "\n";
        cout << "Payment via: " << paymentMethod << "\n";
    }
};


// =======================
// BAD: LSP Violation
// =======================
class Animal {
public:
    virtual string sound() { return "Some sound"; }
    virtual string fly()   { return "I can fly!"; }
    virtual ~Animal() {}
};

class Dog : public Animal {
public:
    string fly() override { throw runtime_error("Dogs can't fly!"); }
};

class Bird : public Animal {
public:
    string fly() override { return "I am flying!"; }
};


// =======================
// BAD: ISP Violation
// =======================
class WorkerInterface {
public:
    virtual void work()          = 0;
    virtual void eat()           = 0;
    virtual void sleep()         = 0;
    virtual void attendMeeting() = 0;
    virtual void writeCode()     = 0;
    virtual ~WorkerInterface() {}
};

class HumanWorker : public WorkerInterface {
public:
    void work()          override { cout << "Human working\n"; }
    void eat()           override { cout << "Human eating\n"; }
    void sleep()         override { cout << "Human sleeping\n"; }
    void attendMeeting() override { cout << "Human in meeting\n"; }
    void writeCode()     override { cout << "Human writing code\n"; }
};

class RobotWorker : public WorkerInterface {
public:
    void work()          override { cout << "Robot working\n"; }
    void eat()           override { throw runtime_error("Robots don't eat!"); }
    void sleep()         override { throw runtime_error("Robots don't sleep!"); }
    void attendMeeting() override { cout << "Robot in meeting\n"; }
    void writeCode()     override { cout << "Robot writing code\n"; }
};


// =======================
// GOOD: DIP — depend on abstraction
// =======================
class IDatabase {
public:
    virtual void save(const string& data) = 0;
    virtual ~IDatabase() {}
};

class MySQLDatabase : public IDatabase {
public:
    void save(const string& data) override {
        cout << "Saving to MySQL: " << data << "\n";
    }
};

class NoSQLDatabase : public IDatabase {
public:
    void save(const string& data) override {
        cout << "Saving to NoSQL: " << data << "\n";
    }
};

class ReportService {
    IDatabase* database;
public:
    ReportService(IDatabase* db) : database(db) {}

    void setDatabase(IDatabase* db) { database = db; }

    void generateReport(const string& data) {
        cout << "Generating report...\n";
        database->save(data);
    }
};


int main() {
    OrderProcessor processor;
    processor.processOrder("PREMIUM", {"LAPTOP", "PHONE"}, "UPI");

    MySQLDatabase mysql;
    ReportService report(&mysql);
    report.generateReport("Order #1001");

    NoSQLDatabase nosql;
    report.setDatabase(&nosql);
    report.generateReport("Order #1002");

    return 0;
}
