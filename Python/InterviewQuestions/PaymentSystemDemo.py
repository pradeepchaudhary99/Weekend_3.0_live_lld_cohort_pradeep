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

# RequestPayment{
#     Payment
#     Idempotency;
# }

import uuid
from abc import ABC, abstractmethod
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime
from enum import Enum, auto
from typing import Dict, Optional


class PaymentStatus(Enum):
    PENDING = auto()
    SUCCESS = auto()
    FAILED = auto()


class Payment:
    def __init__(self, payment_id: str, order_id: str, amount: float):
        self.payment_id = payment_id
        self.order_id = order_id
        self.amount = amount
        self.status = PaymentStatus.PENDING
        self.time = datetime.now()


class PaymentMethod(ABC):
    @abstractmethod
    def validate(self, payment: Payment) -> bool:
        raise NotImplementedError


class PaymentGateway(ABC):
    @abstractmethod
    def process_payment(self, payment: Payment) -> bool:
        raise NotImplementedError


class IdempotencyRepository:
    def __init__(self):
        self.repository: Dict[str, Payment] = {}  # Key: idempotencyKey, Payment


class PaymentRepository:
    def __init__(self):
        self.payment_repository: Dict[str, Payment] = {}  # Key: paymentId, Payment


class PaymentService:
    def __init__(
        self,
        idempotency_repository: IdempotencyRepository,
        payment_repository: PaymentRepository,
        payment_method: PaymentMethod,
        payment_gateway: PaymentGateway,
    ):
        self.idempotency_repository = idempotency_repository
        self.payment_repository = payment_repository
        self.payment_method = payment_method
        self.payment_gateway = payment_gateway
        self.executor = ThreadPoolExecutor()

    def create_payment(self, request) -> Optional[Payment]:
        # Step 1: validate Request for duplicacy
        if request.idempotency_key in self.idempotency_repository.repository:
            return self.idempotency_repository.repository[request.idempotency_key]
        else:
            # create the entry in the Idempotency Repository and store to avoid future duplicate entries
            pass

        payment = Payment(str(uuid.uuid4()), request.order_id, request.amount)  # Distributed Key generator
        self.payment_repository.payment_repository[payment.payment_id] = payment
        self.idempotency_repository.repository[request.idempotency_key] = payment

        if self.payment_method.validate(payment):
            self.executor.submit(self.payment_gateway.process_payment, payment)
        else:
            # failed
            # Undo
            payment.status = PaymentStatus.FAILED

        return payment


def main() -> None:
    pass


if __name__ == "__main__":
    main()
