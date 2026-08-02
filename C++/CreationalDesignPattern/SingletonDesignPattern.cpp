#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class Logger {
    static std::unique_ptr<Logger> instance;
    static std::mutex mtx;
    std::vector<std::string> logs;

    Logger() { std::cout << "object created" << std::endl; }

public:
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void appendLogs(const std::string& log) { logs.push_back(log); }

    static Logger& getInstanceNormalThreadUnsafe() {
        if (!instance) {
            instance = std::unique_ptr<Logger>(new Logger());
        }
        return *instance;
    }

    static Logger& getInstanceSynchronized() {
        std::lock_guard<std::mutex> lock(mtx);
        if (!instance) {
            instance = std::unique_ptr<Logger>(new Logger());
        }
        return *instance;
    }

    static Logger& getInstance() {
        if (!instance) {
            std::lock_guard<std::mutex> lock(mtx);
            if (!instance) {
                instance = std::unique_ptr<Logger>(new Logger());
            }
        }
        return *instance;
    }
};

std::unique_ptr<Logger> Logger::instance = nullptr;
std::mutex Logger::mtx;

int main() {
    // Logger& logger1 = Logger::getInstance();
    // Logger& logger2 = Logger::getInstance();
    // Logger& logger3 = Logger::getInstance();

    auto task1 = []() { Logger::getInstance(); };

    // std::thread thread1(task1);
    // std::thread thread2(task1);
    // thread1.join();
    // thread2.join();

    std::vector<std::thread> threads;
    for (int i = 0; i < 2; i++) {
        threads.emplace_back(task1);
    }
    for (auto& t : threads) {
        t.join();
    }

    return 0;
}
