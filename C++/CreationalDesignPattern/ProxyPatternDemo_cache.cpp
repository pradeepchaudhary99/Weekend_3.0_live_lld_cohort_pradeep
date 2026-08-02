#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

// fun non
// entitieis
//
// relationsji
//
// class Notification
//     - priority
//     - message

struct IRateLimitinStrategy {
    virtual ~IRateLimitinStrategy() = default;
};

struct Notification {
    std::string priority;
    std::string message;
};

struct Database {
    virtual std::string fetchData(const std::string& query) = 0;
    virtual ~Database() = default;
};

struct RealDatabase : Database {
    std::string fetchData(const std::string& query) override {
        std::cout << "Fetching confidential employee salary data..." << std::endl;
        return "data for " + query;
    }
};

class DatabaseProxy : public Database {
    std::optional<std::string> role;
    std::unique_ptr<RealDatabase> database;
    std::unordered_map<std::string, std::string> cache;

public:
    explicit DatabaseProxy(std::optional<std::string> role) : role(std::move(role)) {}

    std::string fetchData(const std::string& query) override {
        auto it = cache.find(query);
        if (it != cache.end()) {
            return it->second;
        }

        if (!database) {
            database = std::make_unique<RealDatabase>();
        }

        std::string value = database->fetchData(query);
        cache[query] = value;
        return value;
    }
};

int main() {
    DatabaseProxy user(std::nullopt);
    user.fetchData("dsadsadasd");

    return 0;
}
