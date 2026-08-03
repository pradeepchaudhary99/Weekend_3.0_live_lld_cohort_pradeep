"""
Zomato Food Delivery Demo
------------------------------

Functional Requirements:
    1. Search/Browse/Recommend by dish or by proximity to the customer's location
    2. Select the restaurant and explore it
    3. Add items to a cart
    4. Place order / make the payment / compute total
    5. Delivery partner assigned
    6. Track the order status

    ------ OUT OF SCOPE -----
    Support multiple types of payments
    Admin Service to manage the CRM

Non-Functional Requirements:
    No double assignment
    No lost/duplicate cart updates
    Valid state machine
    Extensibility

Entities:
    Customer
    FoodDeliveryService
    DeliveryPartner
    DeliveryPartnerAssignmentService
    RestrauntManagerService
    Restraunt, MenuItem
    Cart
    Order, OrderService
    PaymentGateway
"""

import math
import threading
import uuid
from abc import ABC, abstractmethod
from enum import Enum, auto
from typing import Dict, List, Optional


class Location:
    def __init__(self, x: float, y: float):
        self.x = x
        self.y = y

    def distance_to(self, other: "Location") -> float:
        return math.sqrt((self.x - other.x) ** 2 + (self.y - other.y) ** 2)


class MenuItem:
    def __init__(self, name: str, price: float):
        self.id = str(uuid.uuid4())
        self.name = name
        self.price = price
        self.available = True


class Restraunt:
    def __init__(self, name: str, location: Location):
        self.id = str(uuid.uuid4())
        self.name = name
        self.location = location
        self.menu: Dict[str, MenuItem] = {}
        self.open = True

    def add_menu_item(self, item: MenuItem) -> None:
        self.menu[item.id] = item

    def get_menu(self) -> List[MenuItem]:
        return list(self.menu.values())

    def has_dish(self, dish_name: str) -> bool:
        return any(item.available and item.name.lower() == dish_name.lower() for item in self.menu.values())


class RestrauntManagerService:
    def __init__(self):
        self._restraunts: Dict[str, Restraunt] = {}

    def add_restraunt(self, restraunt: Restraunt) -> Restraunt:
        self._restraunts[restraunt.id] = restraunt
        return restraunt

    def search_by_dish(self, dish_name: str) -> List[Restraunt]:
        return [r for r in self._restraunts.values() if r.open and r.has_dish(dish_name)]

    def search_by_proximity(self, customer_location: Location, radius: float) -> List[Restraunt]:
        return [
            r for r in self._restraunts.values()
            if r.open and r.location.distance_to(customer_location) <= radius
        ]


class Cart:
    def __init__(self):
        self.lock = threading.RLock()
        self.restraunt_id: Optional[str] = None
        self.items: Dict[MenuItem, int] = {}

    def add_item(self, restraunt: Restraunt, item: MenuItem, quantity: int) -> None:
        with self.lock:
            if self.restraunt_id is not None and self.restraunt_id != restraunt.id:
                raise ValueError("Cart already has items from another restraunt; clear it first")
            self.restraunt_id = restraunt.id
            self.items[item] = self.items.get(item, 0) + quantity

    def total(self) -> float:
        with self.lock:
            return sum(item.price * qty for item, qty in self.items.items())

    def clear(self) -> None:
        with self.lock:
            self.items.clear()
            self.restraunt_id = None


class Customer:
    def __init__(self, name: str, address: Location):
        self.id = str(uuid.uuid4())
        self.name = name
        self.address = address
        self.cart = Cart()


class DeliveryPartner:
    def __init__(self, name: str, current_location: Location):
        self.id = str(uuid.uuid4())
        self.name = name
        self.current_location = current_location
        self.available = True


class PaymentGateway(ABC):
    @abstractmethod
    def pay(self, amount: float) -> bool:
        pass


class MockPaymentGateway(PaymentGateway):
    def pay(self, amount: float) -> bool:
        print(f"Charged {amount:.2f} via mock payment gateway")
        return True


class OrderStatus(Enum):
    PLACED = auto()
    PARTNER_ASSIGNED = auto()
    PICKED_UP = auto()
    DELIVERED = auto()
    CANCELLED = auto()


class Order:
    def __init__(self, customer: Customer, restraunt: Restraunt, items: Dict[MenuItem, int], total_amount: float):
        self.id = str(uuid.uuid4())
        self.customer = customer
        self.restraunt = restraunt
        self.items = items
        self.total_amount = total_amount
        self.status = OrderStatus.PLACED
        self.delivery_partner: Optional[DeliveryPartner] = None


class DeliveryPartnerAssignmentService:
    """Assigns the nearest available delivery partner to the restraunt; no
    partner is ever handed two active orders at once, which rules out
    double assignment."""

    def __init__(self):
        self._partners: List[DeliveryPartner] = []
        self._lock = threading.Lock()

    def register(self, partner: DeliveryPartner) -> None:
        self._partners.append(partner)

    def assign(self, order: Order) -> Optional[DeliveryPartner]:
        with self._lock:
            nearest = None
            best_distance = float("inf")
            for partner in self._partners:
                if not partner.available:
                    continue
                distance = partner.current_location.distance_to(order.restraunt.location)
                if distance < best_distance:
                    best_distance = distance
                    nearest = partner
            if nearest is not None:
                nearest.available = False
            return nearest

    def release(self, partner: DeliveryPartner) -> None:
        with self._lock:
            partner.available = True


class OrderService:
    def __init__(self, payment_gateway: PaymentGateway, assignment_service: DeliveryPartnerAssignmentService):
        self._payment_gateway = payment_gateway
        self._assignment_service = assignment_service
        self._orders: Dict[str, Order] = {}

    def place_order(self, customer: Customer, restraunt: Restraunt) -> Order:
        cart = customer.cart
        with cart.lock:
            if not cart.items:
                raise ValueError("Cannot place an order with an empty cart")
            order = Order(customer, restraunt, dict(cart.items), cart.total())
            self._orders[order.id] = order
            cart.items.clear()
            cart.restraunt_id = None

        self._payment_gateway.pay(order.total_amount)

        partner = self._assignment_service.assign(order)
        if partner is not None:
            order.delivery_partner = partner
            order.status = OrderStatus.PARTNER_ASSIGNED
        return order

    def advance_status(self, order_id: str, status: OrderStatus) -> None:
        order = self._orders.get(order_id)
        if order is None:
            raise ValueError(f"Unknown order: {order_id}")
        order.status = status
        if status == OrderStatus.DELIVERED and order.delivery_partner is not None:
            self._assignment_service.release(order.delivery_partner)

    def track_order(self, order_id: str) -> Optional[Order]:
        return self._orders.get(order_id)


class FoodDeliveryService:
    def __init__(self):
        self.restraunt_manager_service = RestrauntManagerService()
        self.assignment_service = DeliveryPartnerAssignmentService()
        self.order_service = OrderService(MockPaymentGateway(), self.assignment_service)


def main() -> None:
    service = FoodDeliveryService()

    tasty = service.restraunt_manager_service.add_restraunt(Restraunt("Tasty Bites", Location(0, 0)))
    paneer_tikka = MenuItem("Paneer Tikka", 220.0)
    naan = MenuItem("Butter Naan", 40.0)
    tasty.add_menu_item(paneer_tikka)
    tasty.add_menu_item(naan)

    spice = service.restraunt_manager_service.add_restraunt(Restraunt("Spice Route", Location(5, 5)))
    spice.add_menu_item(MenuItem("Chicken Biryani", 260.0))

    service.assignment_service.register(DeliveryPartner("Ravi", Location(0.5, 0.5)))
    service.assignment_service.register(DeliveryPartner("Neha", Location(4.5, 4.5)))

    alice = Customer("Alice", Location(1, 1))

    print("Search by dish 'Paneer Tikka': ")
    for r in service.restraunt_manager_service.search_by_dish("Paneer Tikka"):
        print(f"  - {r.name}")

    print("Search by proximity (radius 3 from Alice):")
    for r in service.restraunt_manager_service.search_by_proximity(alice.address, 3):
        print(f"  - {r.name}")

    alice.cart.add_item(tasty, paneer_tikka, 1)
    alice.cart.add_item(tasty, naan, 2)
    print(f"Cart total: {alice.cart.total():.2f}")

    order = service.order_service.place_order(alice, tasty)
    partner_name = order.delivery_partner.name if order.delivery_partner else "none"
    print(f"Order placed: {order.id} status={order.status.name} partner={partner_name}")

    service.order_service.advance_status(order.id, OrderStatus.PICKED_UP)
    print(f"Order status: {service.order_service.track_order(order.id).status.name}")

    service.order_service.advance_status(order.id, OrderStatus.DELIVERED)
    print(f"Order status: {service.order_service.track_order(order.id).status.name}")


if __name__ == "__main__":
    main()
