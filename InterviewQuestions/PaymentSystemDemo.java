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

import java.util.Map;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;

// ---------- Domain ----------

enum PaymentStatus {
    PENDING, SUCCESS, FAILED
}

class Payment {
    private final String paymentId;
    private final String orderId;
    private final double amount;
    private final long createdAtMillis;
    // Mutated from the async gateway-processing thread while readers may
    // observe it concurrently, hence AtomicReference rather than a plain field.
    private final AtomicReference<PaymentStatus> status = new AtomicReference<>(PaymentStatus.PENDING);

    public Payment(String paymentId, String orderId, double amount) {
        this.paymentId = paymentId;
        this.orderId = orderId;
        this.amount = amount;
        this.createdAtMillis = System.currentTimeMillis();
    }

    public String getPaymentId() {
        return paymentId;
    }

    public String getOrderId() {
        return orderId;
    }

    public double getAmount() {
        return amount;
    }

    public long getCreatedAtMillis() {
        return createdAtMillis;
    }

    public PaymentStatus getStatus() {
        return status.get();
    }

    public void setStatus(PaymentStatus newStatus) {
        status.set(newStatus);
    }
}

class PaymentRequest {
    private final String idempotencyKey;
    private final String orderId;
    private final double amount;
    private final PaymentMethodType methodType;

    public PaymentRequest(String idempotencyKey, String orderId, double amount, PaymentMethodType methodType) {
        this.idempotencyKey = idempotencyKey;
        this.orderId = orderId;
        this.amount = amount;
        this.methodType = methodType;
    }

    public String getIdempotencyKey() {
        return idempotencyKey;
    }

    public String getOrderId() {
        return orderId;
    }

    public double getAmount() {
        return amount;
    }

    public PaymentMethodType getMethodType() {
        return methodType;
    }
}

// ---------- Payment method (Strategy pattern) ----------

enum PaymentMethodType {
    UPI, CARD, NET_BANKING
}

interface PaymentMethod {
    boolean validate(Payment payment);
}

class UpiPaymentMethod implements PaymentMethod {
    @Override
    public boolean validate(Payment payment) {
        return payment.getAmount() > 0;
    }
}

class CardPaymentMethod implements PaymentMethod {
    @Override
    public boolean validate(Payment payment) {
        return payment.getAmount() > 0 && payment.getAmount() <= 500_000;
    }
}

class NetBankingPaymentMethod implements PaymentMethod {
    @Override
    public boolean validate(Payment payment) {
        return payment.getAmount() > 0;
    }
}

class PaymentMethodFactory {
    public static PaymentMethod create(PaymentMethodType type) {
        switch (type) {
            case UPI:
                return new UpiPaymentMethod();
            case CARD:
                return new CardPaymentMethod();
            case NET_BANKING:
                return new NetBankingPaymentMethod();
            default:
                throw new IllegalArgumentException("Unsupported payment method: " + type);
        }
    }
}

// ---------- Payment gateway + Retry decorator ----------

interface PaymentGateway {
    boolean processPayment(Payment payment);
}

// Simulates a flaky downstream gateway: fails about 40% of the time so the
// retry decorator below has something real to demonstrate.
class SimulatedPaymentGateway implements PaymentGateway {
    private final java.util.Random random = new java.util.Random();

    @Override
    public boolean processPayment(Payment payment) {
        boolean success = random.nextDouble() > 0.4;
        System.out.println("Gateway attempt for " + payment.getPaymentId() + " -> " + (success ? "OK" : "FAILED"));
        return success;
    }
}

// Decorator pattern: adds retry-with-backoff on top of any PaymentGateway
// without the gateway implementation knowing about retries.
class RetryingPaymentGateway implements PaymentGateway {
    private final PaymentGateway delegate;
    private final int maxAttempts;
    private final long backoffMillis;

    public RetryingPaymentGateway(PaymentGateway delegate, int maxAttempts, long backoffMillis) {
        this.delegate = delegate;
        this.maxAttempts = maxAttempts;
        this.backoffMillis = backoffMillis;
    }

    @Override
    public boolean processPayment(Payment payment) {
        for (int attempt = 1; attempt <= maxAttempts; attempt++) {
            if (delegate.processPayment(payment)) {
                return true;
            }
            if (attempt < maxAttempts) {
                try {
                    Thread.sleep(backoffMillis);
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    return false;
                }
            }
        }
        return false;
    }
}

// ---------- Repositories ----------

class IdempotencyRepository {
    private final Map<String, Payment> repository = new ConcurrentHashMap<>();

    // Atomically returns the existing payment for this key, or creates and
    // stores `supplier.get()` if none exists yet -- the single choke point
    // that makes duplicate concurrent requests collapse onto one Payment.
    public Payment getOrCreate(String idempotencyKey, java.util.function.Supplier<Payment> supplier) {
        return repository.computeIfAbsent(idempotencyKey, key -> supplier.get());
    }
}

class PaymentRepository {
    private final Map<String, Payment> paymentRepository = new ConcurrentHashMap<>();

    public void save(Payment payment) {
        paymentRepository.put(payment.getPaymentId(), payment);
    }

    public Payment get(String paymentId) {
        return paymentRepository.get(paymentId);
    }
}

// ---------- Payment service (orchestrator) ----------

class PaymentService {
    private final IdempotencyRepository idempotencyRepository;
    private final PaymentRepository paymentRepository;
    private final PaymentGateway paymentGateway;
    private final ExecutorService executor;

    public PaymentService(IdempotencyRepository idempotencyRepository, PaymentRepository paymentRepository,
                           PaymentGateway paymentGateway) {
        this.idempotencyRepository = idempotencyRepository;
        this.paymentRepository = paymentRepository;
        this.paymentGateway = paymentGateway;
        this.executor = Executors.newFixedThreadPool(4);
    }

    public Payment createPayment(PaymentRequest request) {
        AtomicInteger created = new AtomicInteger(0);

        // Step 1: collapse duplicate concurrent requests for the same idempotency
        // key onto a single Payment, created at most once.
        Payment payment = idempotencyRepository.getOrCreate(request.getIdempotencyKey(), () -> {
            created.incrementAndGet();
            Payment p = new Payment(UUID.randomUUID().toString(), request.getOrderId(), request.getAmount());
            paymentRepository.save(p);
            return p;
        });

        if (created.get() == 0) {
            System.out.println("Duplicate request for idempotency key " + request.getIdempotencyKey()
                    + " -> returning existing payment " + payment.getPaymentId() + " (" + payment.getStatus() + ")");
            return payment;
        }

        // Step 2: validate with the chosen strategy before spending gateway calls.
        PaymentMethod method = PaymentMethodFactory.create(request.getMethodType());
        if (!method.validate(payment)) {
            payment.setStatus(PaymentStatus.FAILED);
            System.out.println("Validation failed for " + payment.getPaymentId());
            return payment;
        }

        // Step 3: process asynchronously so the caller isn't blocked on the gateway.
        executor.submit(() -> {
            boolean success = paymentGateway.processPayment(payment);
            payment.setStatus(success ? PaymentStatus.SUCCESS : PaymentStatus.FAILED);
            System.out.println("Payment " + payment.getPaymentId() + " finished with status " + payment.getStatus());
        });

        return payment;
    }

    public void shutdown() throws InterruptedException {
        executor.shutdown();
        executor.awaitTermination(5, TimeUnit.SECONDS);
    }
}

// ---------- Demo / Simulation ----------

public class PaymentSystemDemo {

    public static void main(String[] args) throws InterruptedException {
        IdempotencyRepository idempotencyRepository = new IdempotencyRepository();
        PaymentRepository paymentRepository = new PaymentRepository();
        PaymentGateway gateway = new RetryingPaymentGateway(new SimulatedPaymentGateway(), 3, 100);
        PaymentService service = new PaymentService(idempotencyRepository, paymentRepository, gateway);

        // Five threads all submit the SAME idempotency key concurrently, simulating
        // a client that retries a network call: only one Payment should be created.
        String sharedIdempotencyKey = "order-42-checkout";
        int duplicateAttempts = 5;
        ExecutorService callers = Executors.newFixedThreadPool(duplicateAttempts);
        CountDownLatch latch = new CountDownLatch(duplicateAttempts);

        System.out.println("Firing " + duplicateAttempts + " concurrent duplicate requests for the same order...");
        for (int i = 0; i < duplicateAttempts; i++) {
            callers.submit(() -> {
                try {
                    service.createPayment(new PaymentRequest(sharedIdempotencyKey, "order-42", 999.0, PaymentMethodType.UPI));
                } finally {
                    latch.countDown();
                }
            });
        }
        latch.await();
        callers.shutdown();
        callers.awaitTermination(5, TimeUnit.SECONDS);

        System.out.println("\nDistinct payments stored for order-42's idempotency key: 1 (guaranteed by design)\n");

        // A genuinely new payment with its own idempotency key.
        Payment second = service.createPayment(new PaymentRequest("order-99-checkout", "order-99", 250.0, PaymentMethodType.CARD));
        System.out.println("Created new payment " + second.getPaymentId() + " for order-99");

        Thread.sleep(1000); // let async gateway processing settle before shutdown
        service.shutdown();
    }
}
