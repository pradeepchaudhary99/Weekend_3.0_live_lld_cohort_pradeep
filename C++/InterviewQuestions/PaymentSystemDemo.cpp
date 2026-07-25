/*
Functional Requirements:
    User should be able to initiate a payment
    Support multiple payment methods [UPI, CARD, NetBanking] :[Strategy]
    ** Prevent duplicate payments {Idempotency}
    Payment Status : PENDING, SUCCESS, FAILED
    Retry Failed Payments : Decorator Design Patterns
    Store payment History


Non-Functional Requirements:
Thread-safe
Extensible
No Duplicate Charges
Async Processing    : ThreadPool we can achieve this



Entities:
PaymentStatus
Payment
PaymentMethod
PaymentService
IdempotencyManager/ IdempotencyRepository

*/

#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// ---------- Domain ----------

enum class PaymentStatus { PENDING, SUCCESS, FAILED };

std::string paymentStatusName(PaymentStatus status) {
    switch (status) {
        case PaymentStatus::PENDING: return "PENDING";
        case PaymentStatus::SUCCESS: return "SUCCESS";
        case PaymentStatus::FAILED: return "FAILED";
    }
    return "UNKNOWN";
}

class Payment {
public:
    Payment(std::string paymentId, std::string orderId, double amount)
        : paymentId_(std::move(paymentId)), orderId_(std::move(orderId)), amount_(amount),
          status_(PaymentStatus::PENDING) {}

    const std::string& getPaymentId() const { return paymentId_; }
    const std::string& getOrderId() const { return orderId_; }
    double getAmount() const { return amount_; }

    PaymentStatus getStatus() {
        std::lock_guard<std::mutex> lock(statusMutex_);
        return status_;
    }

    void setStatus(PaymentStatus status) {
        std::lock_guard<std::mutex> lock(statusMutex_);
        status_ = status;
    }

private:
    std::string paymentId_;
    std::string orderId_;
    double amount_;
    PaymentStatus status_;
    std::mutex statusMutex_;
};

namespace {
std::string generateUuid() {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dist;
    std::ostringstream oss;
    oss << std::hex << dist(gen) << dist(gen);
    return oss.str();
}
}  // namespace

enum class PaymentMethodType { UPI, CARD, NET_BANKING };

struct PaymentRequest {
    std::string idempotencyKey;
    std::string orderId;
    double amount = 0.0;
    PaymentMethodType methodType;
};

// ---------- Payment method (Strategy pattern) ----------

struct PaymentMethod {
    virtual ~PaymentMethod() = default;
    virtual bool validate(const std::shared_ptr<Payment>& payment) = 0;
};

class UpiPaymentMethod : public PaymentMethod {
public:
    bool validate(const std::shared_ptr<Payment>& payment) override { return payment->getAmount() > 0; }
};

class CardPaymentMethod : public PaymentMethod {
public:
    bool validate(const std::shared_ptr<Payment>& payment) override {
        return payment->getAmount() > 0 && payment->getAmount() <= 500000;
    }
};

class NetBankingPaymentMethod : public PaymentMethod {
public:
    bool validate(const std::shared_ptr<Payment>& payment) override { return payment->getAmount() > 0; }
};

class PaymentMethodFactory {
public:
    static std::unique_ptr<PaymentMethod> create(PaymentMethodType type) {
        switch (type) {
            case PaymentMethodType::UPI: return std::make_unique<UpiPaymentMethod>();
            case PaymentMethodType::CARD: return std::make_unique<CardPaymentMethod>();
            case PaymentMethodType::NET_BANKING: return std::make_unique<NetBankingPaymentMethod>();
        }
        throw std::invalid_argument("Unsupported payment method");
    }
};

// ---------- Payment gateway + Retry decorator ----------

struct PaymentGateway {
    virtual ~PaymentGateway() = default;
    virtual bool processPayment(const std::shared_ptr<Payment>& payment) = 0;
};

// Simulates a flaky downstream gateway: fails about 40% of the time so the
// retry decorator below has something real to demonstrate.
class SimulatedPaymentGateway : public PaymentGateway {
public:
    bool processPayment(const std::shared_ptr<Payment>& payment) override {
        static thread_local std::mt19937 rng(std::random_device{}());
        static thread_local std::uniform_real_distribution<double> dist(0.0, 1.0);
        bool success = dist(rng) > 0.4;
        std::cout << ("Gateway attempt for " + payment->getPaymentId() + " -> " + (success ? "OK" : "FAILED") + "\n");
        return success;
    }
};

// Decorator pattern: adds retry-with-backoff on top of any PaymentGateway
// without the gateway implementation knowing about retries.
class RetryingPaymentGateway : public PaymentGateway {
public:
    RetryingPaymentGateway(std::shared_ptr<PaymentGateway> delegate, int maxAttempts, long long backoffMillis)
        : delegate_(std::move(delegate)), maxAttempts_(maxAttempts), backoffMillis_(backoffMillis) {}

    bool processPayment(const std::shared_ptr<Payment>& payment) override {
        for (int attempt = 1; attempt <= maxAttempts_; ++attempt) {
            if (delegate_->processPayment(payment)) {
                return true;
            }
            if (attempt < maxAttempts_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(backoffMillis_));
            }
        }
        return false;
    }

private:
    std::shared_ptr<PaymentGateway> delegate_;
    int maxAttempts_;
    long long backoffMillis_;
};

// ---------- Repositories ----------

class IdempotencyRepository {
public:
    // Atomically returns the existing payment for this key, or creates and
    // stores factory() if none exists yet -- the single choke point that
    // makes duplicate concurrent requests collapse onto one Payment.
    std::pair<std::shared_ptr<Payment>, bool> getOrCreate(
        const std::string& idempotencyKey, const std::function<std::shared_ptr<Payment>()>& factory) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = repository_.find(idempotencyKey);
        if (it != repository_.end()) {
            return {it->second, false};
        }
        auto payment = factory();
        repository_[idempotencyKey] = payment;
        return {payment, true};
    }

private:
    std::unordered_map<std::string, std::shared_ptr<Payment>> repository_;
    std::mutex mutex_;
};

class PaymentRepository {
public:
    void save(const std::shared_ptr<Payment>& payment) {
        std::lock_guard<std::mutex> lock(mutex_);
        paymentRepository_[payment->getPaymentId()] = payment;
    }

    std::shared_ptr<Payment> get(const std::string& paymentId) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = paymentRepository_.find(paymentId);
        return it == paymentRepository_.end() ? nullptr : it->second;
    }

private:
    std::unordered_map<std::string, std::shared_ptr<Payment>> paymentRepository_;
    std::mutex mutex_;
};

// ---------- Payment service (orchestrator) ----------

class PaymentService {
public:
    PaymentService(std::shared_ptr<IdempotencyRepository> idempotencyRepository,
                    std::shared_ptr<PaymentRepository> paymentRepository,
                    std::shared_ptr<PaymentGateway> paymentGateway)
        : idempotencyRepository_(std::move(idempotencyRepository)),
          paymentRepository_(std::move(paymentRepository)),
          paymentGateway_(std::move(paymentGateway)) {}

    std::shared_ptr<Payment> createPayment(const PaymentRequest& request) {
        auto repo = paymentRepository_;
        auto factory = [&request, repo]() {
            auto payment = std::make_shared<Payment>(generateUuid(), request.orderId, request.amount);
            repo->save(payment);
            return payment;
        };

        // Step 1: collapse duplicate concurrent requests for the same idempotency
        // key onto a single Payment, created at most once.
        auto result = idempotencyRepository_->getOrCreate(request.idempotencyKey, factory);
        std::shared_ptr<Payment> payment = result.first;
        bool created = result.second;

        if (!created) {
            std::cout << ("Duplicate request for idempotency key " + request.idempotencyKey +
                          " -> returning existing payment " + payment->getPaymentId() + " (" +
                          paymentStatusName(payment->getStatus()) + ")\n");
            return payment;
        }

        // Step 2: validate with the chosen strategy before spending gateway calls.
        auto method = PaymentMethodFactory::create(request.methodType);
        if (!method->validate(payment)) {
            payment->setStatus(PaymentStatus::FAILED);
            std::cout << ("Validation failed for " + payment->getPaymentId() + "\n");
            return payment;
        }

        // Step 3: process asynchronously so the caller isn't blocked on the gateway.
        auto gateway = paymentGateway_;
        {
            std::lock_guard<std::mutex> lock(pendingMutex_);
            pending_.push_back(std::async(std::launch::async, [gateway, payment]() {
                bool success = gateway->processPayment(payment);
                payment->setStatus(success ? PaymentStatus::SUCCESS : PaymentStatus::FAILED);
                std::cout << ("Payment " + payment->getPaymentId() + " finished with status " +
                              paymentStatusName(payment->getStatus()) + "\n");
            }));
        }

        return payment;
    }

    void shutdown() {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        for (auto& future : pending_) {
            future.wait();
        }
        pending_.clear();
    }

private:
    std::shared_ptr<IdempotencyRepository> idempotencyRepository_;
    std::shared_ptr<PaymentRepository> paymentRepository_;
    std::shared_ptr<PaymentGateway> paymentGateway_;
    std::vector<std::future<void>> pending_;
    std::mutex pendingMutex_;
};

int main() {
    auto idempotencyRepository = std::make_shared<IdempotencyRepository>();
    auto paymentRepository = std::make_shared<PaymentRepository>();
    auto gateway = std::make_shared<RetryingPaymentGateway>(std::make_shared<SimulatedPaymentGateway>(), 3, 100);
    PaymentService service(idempotencyRepository, paymentRepository, gateway);

    // Five threads all submit the SAME idempotency key concurrently, simulating
    // a client that retries a network call: only one Payment should be created.
    const std::string sharedIdempotencyKey = "order-42-checkout";
    const int duplicateAttempts = 5;
    std::cout << "Firing " << duplicateAttempts << " concurrent duplicate requests for the same order...\n";

    std::vector<std::thread> callers;
    for (int i = 0; i < duplicateAttempts; ++i) {
        callers.emplace_back([&service, sharedIdempotencyKey]() {
            PaymentRequest request{sharedIdempotencyKey, "order-42", 999.0, PaymentMethodType::UPI};
            service.createPayment(request);
        });
    }
    for (auto& t : callers) {
        t.join();
    }

    std::cout << "\nDistinct payments stored for order-42's idempotency key: 1 (guaranteed by design)\n\n";

    // A genuinely new payment with its own idempotency key.
    PaymentRequest secondRequest{"order-99-checkout", "order-99", 250.0, PaymentMethodType::CARD};
    auto second = service.createPayment(secondRequest);
    std::cout << "Created new payment " << second->getPaymentId() << " for order-99\n";

    service.shutdown();
    return 0;
}
