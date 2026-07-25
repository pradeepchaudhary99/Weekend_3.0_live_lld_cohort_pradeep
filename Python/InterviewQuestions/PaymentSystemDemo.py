"""
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

"""

import random
import threading
import time
import uuid
from abc import ABC, abstractmethod
from concurrent.futures import ThreadPoolExecutor
from enum import Enum, auto
from typing import Callable, Dict, Optional


# ---------- Domain ----------

class PaymentStatus(Enum):
    PENDING = auto()
    SUCCESS = auto()
    FAILED = auto()


class Payment:
    def __init__(self, payment_id: str, order_id: str, amount: float):
        self.payment_id = payment_id
        self.order_id = order_id
        self.amount = amount
        self.created_at = time.time()
        self._status = PaymentStatus.PENDING
        self._status_lock = threading.Lock()

    @property
    def status(self) -> PaymentStatus:
        with self._status_lock:
            return self._status

    @status.setter
    def status(self, new_status: PaymentStatus) -> None:
        with self._status_lock:
            self._status = new_status


class PaymentRequest:
    def __init__(self, idempotency_key: str, order_id: str, amount: float, method_type: "PaymentMethodType"):
        self.idempotency_key = idempotency_key
        self.order_id = order_id
        self.amount = amount
        self.method_type = method_type


# ---------- Payment method (Strategy pattern) ----------

class PaymentMethodType(Enum):
    UPI = auto()
    CARD = auto()
    NET_BANKING = auto()


class PaymentMethod(ABC):
    @abstractmethod
    def validate(self, payment: Payment) -> bool:
        raise NotImplementedError


class UpiPaymentMethod(PaymentMethod):
    def validate(self, payment: Payment) -> bool:
        return payment.amount > 0


class CardPaymentMethod(PaymentMethod):
    def validate(self, payment: Payment) -> bool:
        return 0 < payment.amount <= 500_000


class NetBankingPaymentMethod(PaymentMethod):
    def validate(self, payment: Payment) -> bool:
        return payment.amount > 0


class PaymentMethodFactory:
    @staticmethod
    def create(method_type: PaymentMethodType) -> PaymentMethod:
        if method_type == PaymentMethodType.UPI:
            return UpiPaymentMethod()
        if method_type == PaymentMethodType.CARD:
            return CardPaymentMethod()
        if method_type == PaymentMethodType.NET_BANKING:
            return NetBankingPaymentMethod()
        raise ValueError(f"Unsupported payment method: {method_type}")


# ---------- Payment gateway + Retry decorator ----------

class PaymentGateway(ABC):
    @abstractmethod
    def process_payment(self, payment: Payment) -> bool:
        raise NotImplementedError


class SimulatedPaymentGateway(PaymentGateway):
    """Simulates a flaky downstream gateway: fails about 40% of the time so
    the retry decorator below has something real to demonstrate."""

    def process_payment(self, payment: Payment) -> bool:
        success = random.random() > 0.4
        print(f"Gateway attempt for {payment.payment_id} -> {'OK' if success else 'FAILED'}")
        return success


class RetryingPaymentGateway(PaymentGateway):
    """Decorator pattern: adds retry-with-backoff on top of any PaymentGateway
    without the gateway implementation knowing about retries."""

    def __init__(self, delegate: PaymentGateway, max_attempts: int, backoff_seconds: float):
        self._delegate = delegate
        self._max_attempts = max_attempts
        self._backoff_seconds = backoff_seconds

    def process_payment(self, payment: Payment) -> bool:
        for attempt in range(1, self._max_attempts + 1):
            if self._delegate.process_payment(payment):
                return True
            if attempt < self._max_attempts:
                time.sleep(self._backoff_seconds)
        return False


# ---------- Repositories ----------

class IdempotencyRepository:
    def __init__(self):
        self._repository: Dict[str, Payment] = {}
        self._lock = threading.Lock()

    def get_or_create(self, idempotency_key: str, factory: Callable[[], Payment]) -> Payment:
        """Atomically returns the existing payment for this key, or creates and
        stores factory() if none exists yet -- the single choke point that
        makes duplicate concurrent requests collapse onto one Payment."""
        with self._lock:
            existing = self._repository.get(idempotency_key)
            if existing is not None:
                return existing
            payment = factory()
            self._repository[idempotency_key] = payment
            return payment


class PaymentRepository:
    def __init__(self):
        self._payment_repository: Dict[str, Payment] = {}
        self._lock = threading.Lock()

    def save(self, payment: Payment) -> None:
        with self._lock:
            self._payment_repository[payment.payment_id] = payment

    def get(self, payment_id: str) -> Optional[Payment]:
        with self._lock:
            return self._payment_repository.get(payment_id)


# ---------- Payment service (orchestrator) ----------

class PaymentService:
    def __init__(self, idempotency_repository: IdempotencyRepository, payment_repository: PaymentRepository,
                 payment_gateway: PaymentGateway):
        self._idempotency_repository = idempotency_repository
        self._payment_repository = payment_repository
        self._payment_gateway = payment_gateway
        self._executor = ThreadPoolExecutor(max_workers=4)

    def create_payment(self, request: PaymentRequest) -> Payment:
        created = False

        def factory() -> Payment:
            nonlocal created
            created = True
            payment = Payment(str(uuid.uuid4()), request.order_id, request.amount)
            self._payment_repository.save(payment)
            return payment

        # Step 1: collapse duplicate concurrent requests for the same idempotency
        # key onto a single Payment, created at most once.
        payment = self._idempotency_repository.get_or_create(request.idempotency_key, factory)

        if not created:
            print(f"Duplicate request for idempotency key {request.idempotency_key} "
                  f"-> returning existing payment {payment.payment_id} ({payment.status.name})")
            return payment

        # Step 2: validate with the chosen strategy before spending gateway calls.
        method = PaymentMethodFactory.create(request.method_type)
        if not method.validate(payment):
            payment.status = PaymentStatus.FAILED
            print(f"Validation failed for {payment.payment_id}")
            return payment

        # Step 3: process asynchronously so the caller isn't blocked on the gateway.
        def process() -> None:
            success = self._payment_gateway.process_payment(payment)
            payment.status = PaymentStatus.SUCCESS if success else PaymentStatus.FAILED
            print(f"Payment {payment.payment_id} finished with status {payment.status.name}")

        self._executor.submit(process)
        return payment

    def shutdown(self) -> None:
        self._executor.shutdown(wait=True)


def main() -> None:
    idempotency_repository = IdempotencyRepository()
    payment_repository = PaymentRepository()
    gateway = RetryingPaymentGateway(SimulatedPaymentGateway(), max_attempts=3, backoff_seconds=0.1)
    service = PaymentService(idempotency_repository, payment_repository, gateway)

    # Five threads all submit the SAME idempotency key concurrently, simulating
    # a client that retries a network call: only one Payment should be created.
    shared_idempotency_key = "order-42-checkout"
    duplicate_attempts = 5
    print(f"Firing {duplicate_attempts} concurrent duplicate requests for the same order...")

    with ThreadPoolExecutor(max_workers=duplicate_attempts) as callers:
        futures = [
            callers.submit(service.create_payment,
                            PaymentRequest(shared_idempotency_key, "order-42", 999.0, PaymentMethodType.UPI))
            for _ in range(duplicate_attempts)
        ]
        for future in futures:
            future.result()

    print("\nDistinct payments stored for order-42's idempotency key: 1 (guaranteed by design)\n")

    # A genuinely new payment with its own idempotency key.
    second = service.create_payment(PaymentRequest("order-99-checkout", "order-99", 250.0, PaymentMethodType.CARD))
    print(f"Created new payment {second.payment_id} for order-99")

    time.sleep(1)  # let async gateway processing settle before shutdown
    service.shutdown()


if __name__ == "__main__":
    main()
