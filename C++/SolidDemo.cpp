// SOLID Principles Demonstration
#include <iostream>
#include <string>
using namespace std;

// =======================
// 1. SINGLE RESPONSIBILITY PRINCIPLE (SRP)
// =======================

// BAD
class UserBad {
public:
    string name;
    void saveToDB()   { cout << "Saving user to DB\n"; }
    void sendEmail()  { cout << "Sending email\n"; }
};

// GOOD
class User {
public:
    string name;
    User(string name) : name(name) {}
};

class UserRepository {
public:
    void save(const User& user) { cout << "Saving user to DB\n"; }
};

class EmailService {
public:
    void sendEmail(const User& user) { cout << "Sending email\n"; }
};


// =======================
// 2. OPEN CLOSED PRINCIPLE (OCP)
// =======================

// BAD
class DiscountCalculatorBad {
public:
    double calculate(const string& type) {
        if (type == "NEW")     return 10;
        if (type == "PREMIUM") return 20;
        if (type == "DIWALI")  return 30;
        return 0;
    }
};

// GOOD
class DiscountStrategy {
public:
    virtual double calculate() = 0;
    virtual ~DiscountStrategy() {}
};

class NewCustomerDiscount : public DiscountStrategy {
public:
    double calculate() override { return 10; }
};

class PremiumCustomerDiscount : public DiscountStrategy {
public:
    double calculate() override { return 20; }
};

class DiwaliDiscount : public DiscountStrategy {
public:
    double calculate() override { return 30; }
};

class DiscountCalculator {
public:
    double calculate(DiscountStrategy* strategy) { return strategy->calculate(); }
};


// =======================
// 3. LISKOV SUBSTITUTION PRINCIPLE (LSP)
// =======================

// BAD
class BirdBad {
public:
    virtual void fly() { cout << "Flying\n"; }
};

class PenguinBad : public BirdBad {
public:
    void fly() override { throw runtime_error("Can't fly"); }
};

// GOOD
class Bird {
public:
    virtual void eat()   = 0;
    virtual void sleep() = 0;
    virtual ~Bird() {}
};

class FlyingBird : public Bird {
public:
    virtual void fly() = 0;
};

class Sparrow : public FlyingBird {
public:
    void eat()   override { cout << "Sparrow eating\n"; }
    void sleep() override { cout << "Sparrow sleeping\n"; }
    void fly()   override { cout << "Sparrow flying\n"; }
};

class Penguin : public Bird {
public:
    void eat()   override { cout << "Penguin eating\n"; }
    void sleep() override { cout << "Penguin sleeping\n"; }
};


// =======================
// 4. INTERFACE SEGREGATION PRINCIPLE (ISP)
// =======================

// BAD
class WorkerBad {
public:
    virtual void work() = 0;
    virtual void eat()  = 0;
};

class RobotBad : public WorkerBad {
public:
    void work() override { cout << "Working\n"; }
    void eat()  override { throw runtime_error("Robot doesn't eat"); }
};

// GOOD
class Workable {
public:
    virtual void work() = 0;
    virtual ~Workable() {}
};

class Eatable {
public:
    virtual void eat() = 0;
    virtual ~Eatable() {}
};

class Human : public Workable, public Eatable {
public:
    void work() override { cout << "Working\n"; }
    void eat()  override { cout << "Eating\n"; }
};

class Robot : public Workable {
public:
    void work() override { cout << "Working\n"; }
};


// =======================
// 5. DEPENDENCY INVERSION PRINCIPLE (DIP)
// =======================

// BAD
class MySQLDatabaseBad {
public:
    void connect() { cout << "Connecting to MySQL\n"; }
};

class ApplicationBad {
    MySQLDatabaseBad db;
public:
    void start() { db.connect(); }
};

// GOOD
class Database {
public:
    virtual void connect() = 0;
    virtual ~Database() {}
};

class MySQL : public Database {
public:
    void connect() override { cout << "Connecting to MySQL\n"; }
};

class PostgreSQL : public Database {
public:
    void connect() override { cout << "Connecting to PostgreSQL\n"; }
};

class Application {
    Database* db;
public:
    Application(Database* db) : db(db) {}
    void start() { db->connect(); }
};


// =======================
// MAIN (TESTING)
// =======================
int main() {
    // SRP
    User user("Pradeep");
    UserRepository().save(user);
    EmailService().sendEmail(user);

    // OCP
    DiscountCalculator calc;
    PremiumCustomerDiscount pcd;
    cout << calc.calculate(&pcd) << "\n";

    // LSP
    Sparrow sparrow;
    sparrow.fly();

    // ISP
    Robot robot;
    robot.work();

    // DIP
    MySQL mysql;
    Application app(&mysql);
    app.start();

    return 0;
}
