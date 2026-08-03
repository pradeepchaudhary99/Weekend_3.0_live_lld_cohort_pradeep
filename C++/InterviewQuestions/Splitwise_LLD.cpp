/*
SplitWise System / App
------------------------------

Functional Requirements:
    Create Users
    Create Groups
    Add Expenses in the system
    Split Types
        - Equal Split
        - Percentage Split
    Maintain balances
        balances[A][B] --> what B owes A in A's balance sheet
    Show balances of a User
    should support payments

Non-Functional:
    Thread-Safety
    Consistency across the balance sheet (ACID Compliance)
    Extensibility to support multiple types of splits/expense

----- Out of Scope -------
    Simplification feature of splitwise: Graph Algorithm using Priority Queue
    Settle

Entities:
    User
    Group
    Expense
    SplitStrategy (Interface)
    EqualSplit
    PercentageSplit
    SplitWiseService
    BalanceManagerService
*/

#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class User {
public:
    User(std::string id, std::string name) : id_(std::move(id)), name_(std::move(name)) {}

    const std::string& getId() const { return id_; }
    const std::string& getName() const { return name_; }

private:
    std::string id_;
    std::string name_;
};

struct Split {
    std::shared_ptr<User> paidBy;
    std::shared_ptr<User> paidFor;
    double splitAmount;
    std::string expenseId;
};

class Expense;

struct SplitStrategy {
    virtual ~SplitStrategy() = default;
    virtual std::vector<Split> generateSplit(const Expense& expense) = 0;
};

class Expense {
public:
    Expense(std::string id, std::shared_ptr<User> paidBy, double amount,
            std::vector<std::shared_ptr<User>> usersParticipated, std::shared_ptr<SplitStrategy> strategy)
        : id(std::move(id)), paidBy(std::move(paidBy)), amount(amount),
          usersParticipated(std::move(usersParticipated)), strategy(std::move(strategy)) {}

    std::vector<Split> calculateShare() {
        splits = strategy->generateSplit(*this);
        return splits;
    }

    std::string id;
    std::shared_ptr<User> paidBy;
    double amount;
    std::vector<std::shared_ptr<User>> usersParticipated;
    std::shared_ptr<SplitStrategy> strategy;
    std::vector<Split> splits;
};

// Splits the amount equally among every participant except the payer.
class EqualSplit : public SplitStrategy {
public:
    std::vector<Split> generateSplit(const Expense& expense) override {
        std::vector<Split> splits;
        if (expense.usersParticipated.empty()) {
            return splits;
        }
        double share = expense.amount / static_cast<double>(expense.usersParticipated.size());
        for (const auto& user : expense.usersParticipated) {
            if (user->getId() == expense.paidBy->getId()) {
                continue;
            }
            splits.push_back(Split{expense.paidBy, user, share, expense.id});
        }
        return splits;
    }
};

// Splits the amount according to a percentage assigned to each participant.
class PercentageSplit : public SplitStrategy {
public:
    explicit PercentageSplit(std::unordered_map<std::string, double> percentages)
        : percentages_(std::move(percentages)) {}

    std::vector<Split> generateSplit(const Expense& expense) override {
        std::vector<Split> splits;
        for (const auto& user : expense.usersParticipated) {
            if (user->getId() == expense.paidBy->getId()) {
                continue;
            }
            double percentage = 0.0;
            auto it = percentages_.find(user->getId());
            if (it != percentages_.end()) {
                percentage = it->second;
            }
            splits.push_back(Split{expense.paidBy, user, expense.amount * percentage / 100.0, expense.id});
        }
        return splits;
    }

private:
    std::unordered_map<std::string, double> percentages_;
};

struct Group {
    std::string id;
    std::string name;
    std::vector<std::shared_ptr<User>> users;
    std::vector<std::shared_ptr<Expense>> expenses;
};

// balances_[a][b] is what user b owes user a.
class BalanceManagerService {
public:
    void updateBalanceSheet(const Expense& /*expense*/, const std::vector<Split>& splits) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& split : splits) {
            const std::string& payerId = split.paidBy->getId();
            const std::string& debtorId = split.paidFor->getId();

            balances_[payerId][debtorId] += split.splitAmount;
            balances_[debtorId][payerId] -= split.splitAmount;
        }
    }

    std::unordered_map<std::string, double> getBalance(const std::string& userId) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = balances_.find(userId);
        if (it == balances_.end()) {
            return {};
        }
        return it->second;
    }

private:
    std::unordered_map<std::string, std::unordered_map<std::string, double>> balances_;
    std::mutex mutex_;
};

class SplitWiseService {
public:
    std::shared_ptr<User> createUser(const std::string& userId, const std::string& name) {
        auto user = std::make_shared<User>(userId, name);
        users_[userId] = user;
        return user;
    }

    std::shared_ptr<Group> createGroup(const std::string& groupId, const std::string& name,
                                        std::vector<std::shared_ptr<User>> users) {
        auto group = std::make_shared<Group>();
        group->id = groupId;
        group->name = name;
        group->users = std::move(users);
        groups_[groupId] = group;
        return group;
    }

    std::shared_ptr<Expense> addExpense(const std::string& expenseId, std::shared_ptr<User> paidBy,
                                         double amount, std::vector<std::shared_ptr<User>> usersParticipated,
                                         std::shared_ptr<SplitStrategy> strategy) {
        auto expense = std::make_shared<Expense>(expenseId, std::move(paidBy), amount,
                                                  std::move(usersParticipated), std::move(strategy));
        auto splits = expense->calculateShare();
        balanceManagerService_.updateBalanceSheet(*expense, splits);
        return expense;
    }

    void showBalance(const User& user) {
        auto balances = balanceManagerService_.getBalance(user.getId());
        bool anyNonZero = false;
        for (const auto& [otherId, amount] : balances) {
            if (amount == 0) {
                continue;
            }
            anyNonZero = true;
            std::cout << std::fixed << std::setprecision(2);
            if (amount > 0) {
                std::cout << otherId << " owes " << user.getName() << " " << amount << "\n";
            } else {
                std::cout << user.getName() << " owes " << otherId << " " << -amount << "\n";
            }
        }
        if (!anyNonZero) {
            std::cout << "No pending balances for " << user.getName() << "\n";
        }
    }

private:
    std::unordered_map<std::string, std::shared_ptr<User>> users_;
    std::unordered_map<std::string, std::shared_ptr<Group>> groups_;
    BalanceManagerService balanceManagerService_;
};

int main() {
    SplitWiseService service;

    auto alice = service.createUser("u1", "Alice");
    auto bob = service.createUser("u2", "Bob");
    auto carol = service.createUser("u3", "Carol");

    service.createGroup("g1", "Goa Trip", {alice, bob, carol});

    std::cout << "Equal split: Alice pays 300 for all three\n";
    service.addExpense("e1", alice, 300.0, {alice, bob, carol}, std::make_shared<EqualSplit>());

    std::cout << "Percentage split: Bob pays 200, Alice 30%, Carol 70%\n";
    service.addExpense("e2", bob, 200.0, {alice, bob, carol},
                        std::make_shared<PercentageSplit>(
                            std::unordered_map<std::string, double>{{"u1", 30.0}, {"u3", 70.0}}));

    for (const auto& user : {alice, bob, carol}) {
        std::cout << "\nBalances for " << user->getName() << ":\n";
        service.showBalance(*user);
    }

    return 0;
}
