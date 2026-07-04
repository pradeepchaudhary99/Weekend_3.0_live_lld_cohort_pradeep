#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
using namespace std;

struct IRateLimitinStrategy {
    virtual ~IRateLimitinStrategy() = default;
};

struct Notification {
    string priority;
    string message;
};

// Subject
struct Database {
    virtual string fetchData(const string& query) = 0;
    virtual ~Database() = default;
};

// Real Subject
struct RealDatabase : Database {
    string fetchData(const string& query) override {
        cout << "Fetching confidential employee salary data...\n";
        return "result for '" + query + "'";
    }
};

// Proxy
class DatabaseProxy : public Database {
    string role;
    unique_ptr<RealDatabase> database;
    unordered_map<string, string> cache;

public:
    explicit DatabaseProxy(const string& role) : role(role) {}

    string fetchData(const string& query) override {
        auto it = cache.find(query);
        if (it != cache.end()) {
            return it->second;
        }

        if (!database) {
            database = make_unique<RealDatabase>();
        }

        string value = database->fetchData(query);
        cache[query] = value;
        return value;
    }
};

int main() {
    unique_ptr<Database> user = make_unique<DatabaseProxy>("");
    cout << user->fetchData("dsadsadasd") << "\n";
    return 0;
}
