// Singleton Design Pattern
#include <iostream>
#include <vector>
#include <string>
#include <mutex>
#include <thread>
using namespace std;

class Logger {
    static Logger* instance;
    static mutex mtx;

    Logger() {
        cout << "object created\n";
    }

public:
    vector<string> logs;

    // Thread-unsafe version
    static Logger* getInstanceThreadUnsafe() {
        if (instance == nullptr) {
            instance = new Logger();
        }
        return instance;
    }

    // Thread-safe: lock on every call
    static Logger* getInstanceSynchronized() {
        lock_guard<mutex> lock(mtx);
        if (instance == nullptr) {
            instance = new Logger();
        }
        return instance;
    }

    // Thread-safe: double-checked locking
    static Logger* getInstance() {
        if (instance == nullptr) {
            lock_guard<mutex> lock(mtx);
            if (instance == nullptr) {
                instance = new Logger();
            }
        }
        return instance;
    }

    void appendLogs(const string& log) {
        logs.push_back(log);
    }
};

Logger* Logger::instance = nullptr;
mutex Logger::mtx;

int main() {
    auto task = []() {
        Logger::getInstance();
    };

    vector<thread> threads;
    for (int i = 0; i < 2; i++) {
        threads.emplace_back(task);
    }
    for (auto& t : threads) {
        t.join();
    }

    return 0;
}
