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

// RequestPayment{
//     Payment
//     Idempotency;
// }

#include <chrono>
#include <future>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>

enum class PaymentStatus { PENDING, SUCCESS, FAILED };

class Payment {
public:
    Payment(std::string paymentId, std::string orderId, double amount)
        : paymentId_(std::move(paymentId)), orderId_(std::move(orderId)), amount_(amount),
          status_(PaymentStatus::PENDING), time_(std::chrono::system_clock::now()) {}

    std::string paymentId_;
    std::string orderId_;
    double amount_;
    PaymentStatus status_;
    std::chrono::system_clock::time_point time_;
};

struct PaymentMethod {
    virtual ~PaymentMethod() = default;
    virtual bool validate(const std::shared_ptr<Payment>& payment) = 0;
};

struct PaymentGateway {
    virtual ~PaymentGateway() = default;
    virtual bool processPayment(const std::shared_ptr<Payment>& payment) = 0;
};

class IdempotencyRepository {
public:
    std::unordered_map<std::string, std::shared_ptr<Payment>> repository_;  // Key: idempotencyKey, Payment
};

class PaymentRepository {
public:
    std::unordered_map<std::string, std::shared_ptr<Payment>> paymentRepository_;  // Key: paymentId, Payment
};

struct PaymentRequest {
    std::string idempotencyKey;
    std::string orderId;
    double amount = 0.0;
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

class PaymentService {
public:
    PaymentService(std::shared_ptr<IdempotencyRepository> idempotencyRepository,
                    std::shared_ptr<PaymentRepository> paymentRepository,
                    std::shared_ptr<PaymentMethod> paymentMethod,
                    std::shared_ptr<PaymentGateway> paymentGateway)
        : idempotencyRepository_(std::move(idempotencyRepository)),
          paymentRepository_(std::move(paymentRepository)),
          paymentMethod_(std::move(paymentMethod)),
          paymentGateway_(std::move(paymentGateway)) {}

    std::shared_ptr<Payment> createPayment(const PaymentRequest& request) {
        // Step 1: validate Request for duplicacy
        auto it = idempotencyRepository_->repository_.find(request.idempotencyKey);
        if (it != idempotencyRepository_->repository_.end()) {
            return it->second;
        }
        // create the entry in the Idempotency Repository and store to avoid future duplicate entries

        auto payment = std::make_shared<Payment>(generateUuid(), request.orderId, request.amount);
        paymentRepository_->paymentRepository_[payment->paymentId_] = payment;
        idempotencyRepository_->repository_[request.idempotencyKey] = payment;

        if (paymentMethod_->validate(payment)) {
            std::async(std::launch::async, [this, payment]() {
                paymentGateway_->processPayment(payment);
            });
        } else {
            // failed
            // Undo
            payment->status_ = PaymentStatus::FAILED;
        }

        return payment;
    }

private:
    std::shared_ptr<IdempotencyRepository> idempotencyRepository_;
    std::shared_ptr<PaymentRepository> paymentRepository_;
    std::shared_ptr<PaymentMethod> paymentMethod_;
    std::shared_ptr<PaymentGateway> paymentGateway_;
};

int main() {
    return 0;
}
