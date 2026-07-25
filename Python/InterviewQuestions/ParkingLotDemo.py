"""
FR:

The parkinglot has multiple floors/levels, each level with fixed/variable number of spots/slots
Spots should support multiple types of vehicle: MOTORCYCLE, LARGE, COMPACT, HANDICAPPED

Different Vehicle types are allowed: MOTORCYCLE, CAR, TRUCK
System Should generate ticket at the time of Entry Gate
System Should assign one Slot at some level for the vehicle
System should have multiple strategies for spot selection/assignment
System should support multiple payment methods at exit gate

parkingLot should display the availability at each level
---- Add some decorator Design pattern use case ------


NFR:
 Thread-safety
 Extensibility   : new Vehicle types, spots types, assignment strategies....


//Entry Exit Gate


Listing Down the Entities:
Vehicle(abstract) /Car, Motorcyle, Truck
ParkingSpot
ParkingLevel
ParkingLotManager ---> Orchestrator
SpotAssignmentStrategy
    nearestSpotStartegy
    FirstFitStrategy
    NearestToExitGateStrategy
TicketGenerator : Ticket
Ticket
PaymentProcessor
EntryGate
ExitGate

Display *** Observer Design pattern ***
PaymentStrategy decorated with MembershipDiscountDecorator *** Decorator Design pattern ***
"""

from __future__ import annotations

import itertools
import threading
import time
from abc import ABC, abstractmethod
from concurrent.futures import ThreadPoolExecutor
from enum import Enum, auto
from typing import Dict, List, Optional


class VehicleType(Enum):
    MOTORCYCLE = auto()
    CAR = auto()
    TRUCK = auto()


class SpotType(Enum):
    SMALL = auto()
    MEDIUM = auto()
    LARGE = auto()


class Vehicle(ABC):
    def __init__(self, licence_plate: str, vehicle_type: VehicleType):
        self._licence_plate = licence_plate
        self._type = vehicle_type

    def get_licence_plate(self) -> str:
        return self._licence_plate

    def get_type(self) -> VehicleType:
        return self._type

    @abstractmethod
    def can_fit_in(self, spot_type: SpotType) -> bool:
        raise NotImplementedError


class Motorcycle(Vehicle):
    def __init__(self, licence_plate: str):
        super().__init__(licence_plate, VehicleType.MOTORCYCLE)

    def can_fit_in(self, spot_type: SpotType) -> bool:
        return True


class Car(Vehicle):
    def __init__(self, licence_plate: str):
        super().__init__(licence_plate, VehicleType.CAR)

    def can_fit_in(self, spot_type: SpotType) -> bool:
        return spot_type in (SpotType.MEDIUM, SpotType.LARGE)


class Truck(Vehicle):
    def __init__(self, licence_plate: str):
        super().__init__(licence_plate, VehicleType.TRUCK)

    def can_fit_in(self, spot_type: SpotType) -> bool:
        return spot_type == SpotType.LARGE


class ParkingSpot:
    """Occupancy is claimed with try_park(), guarded by a per-spot lock so two
    threads racing for the same spot can never both succeed."""

    def __init__(self, spot_id: str, level_number: int, spot_type: SpotType):
        self.spot_id = spot_id
        self.level_number = level_number
        self.spot_type = spot_type
        self._vehicle: Optional[Vehicle] = None
        self._lock = threading.Lock()

    def is_occupied(self) -> bool:
        with self._lock:
            return self._vehicle is not None

    def try_park(self, candidate: Vehicle) -> bool:
        with self._lock:
            if self._vehicle is not None:
                return False
            self._vehicle = candidate
            return True

    def unpark(self) -> Optional[Vehicle]:
        with self._lock:
            parked = self._vehicle
            self._vehicle = None
            return parked


class ParkingLevel:
    def __init__(self, level_number: int, spots: List[ParkingSpot]):
        self.level_number = level_number
        self.spots = spots

    def available_count(self, spot_type: SpotType) -> int:
        return sum(1 for s in self.spots if s.spot_type == spot_type and not s.is_occupied())


class SpotAssignmentStrategy(ABC):
    @abstractmethod
    def candidate_spots(self, levels: List[ParkingLevel], vehicle: Vehicle) -> List[ParkingSpot]:
        raise NotImplementedError


class FirstFitStrategy(SpotAssignmentStrategy):
    def candidate_spots(self, levels: List[ParkingLevel], vehicle: Vehicle) -> List[ParkingSpot]:
        return [spot for level in levels for spot in level.spots if vehicle.can_fit_in(spot.spot_type)]


class BestFitStrategy(SpotAssignmentStrategy):
    """Prefers the smallest spot type the vehicle fits in, to save larger
    spots for vehicles that actually need them."""

    _SIZE_ORDER = (SpotType.SMALL, SpotType.MEDIUM, SpotType.LARGE)

    def candidate_spots(self, levels: List[ParkingLevel], vehicle: Vehicle) -> List[ParkingSpot]:
        candidates: List[ParkingSpot] = []
        for spot_type in self._SIZE_ORDER:
            if not vehicle.can_fit_in(spot_type):
                continue
            for level in levels:
                candidates.extend(spot for spot in level.spots if spot.spot_type == spot_type)
        return candidates


class Ticket:
    def __init__(self, ticket_id: str, vehicle: Vehicle, spot: ParkingSpot, entry_time: float):
        self.ticket_id = ticket_id
        self.vehicle = vehicle
        self.spot = spot
        self.entry_time = entry_time


class TicketGenerator:
    def __init__(self):
        self._counter = itertools.count(1)
        self._lock = threading.Lock()

    def next_ticket_id(self) -> str:
        with self._lock:
            return f"T-{next(self._counter)}"


class PaymentStrategy(ABC):
    @abstractmethod
    def calculate_and_charge(self, ticket: Ticket, exit_time: float) -> float:
        raise NotImplementedError


class HourlyRatePayment(PaymentStrategy):
    RATE_PER_HOUR = 20.0

    def calculate_and_charge(self, ticket: Ticket, exit_time: float) -> float:
        duration_seconds = max(0.0, exit_time - ticket.entry_time)
        hours = max(1.0, duration_seconds / 3600.0)  # minimum 1 hour billed
        amount = hours * self.RATE_PER_HOUR
        print(f"Charging {amount:.2f} for ticket {ticket.ticket_id} ({hours:.2f} hours)")
        return amount


class PaymentDecorator(PaymentStrategy):
    """Decorator pattern: wraps any PaymentStrategy to add behavior without
    the base strategy knowing anything about it."""

    def __init__(self, delegate: PaymentStrategy):
        self._delegate = delegate


class MembershipDiscountDecorator(PaymentDecorator):
    def __init__(self, delegate: PaymentStrategy, discount_percent: float):
        super().__init__(delegate)
        self._discount_percent = discount_percent

    def calculate_and_charge(self, ticket: Ticket, exit_time: float) -> float:
        base = self._delegate.calculate_and_charge(ticket, exit_time)
        discounted = base * (1 - self._discount_percent / 100.0)
        print(f"Applying {self._discount_percent:.0f}% membership discount -> {discounted:.2f}")
        return discounted


class DisplayDevice(ABC):
    @abstractmethod
    def update(self, data: str) -> None:
        raise NotImplementedError


class Screen(DisplayDevice):
    def __init__(self, name: str):
        self._name = name

    def update(self, data: str) -> None:
        print(f"[{self._name}] {data}")


class ParkingLotManager:
    """Races candidate spots one at a time: try_park() is atomic per-spot,
    so concurrent callers can never double-park the same spot, and no global
    lock is needed across the whole manager."""

    def __init__(self, levels: List[ParkingLevel], assignment_strategy: SpotAssignmentStrategy):
        self._levels = levels
        self._assignment_strategy = assignment_strategy
        self._display_devices: List[DisplayDevice] = []
        self._active_tickets: Dict[str, Ticket] = {}
        self._tickets_lock = threading.Lock()

    def add_display_device(self, device: DisplayDevice) -> None:
        self._display_devices.append(device)

    def park_vehicle(self, vehicle: Vehicle) -> Optional[ParkingSpot]:
        candidates = self._assignment_strategy.candidate_spots(self._levels, vehicle)
        for spot in candidates:
            if spot.try_park(vehicle):
                self.update_display()
                return spot
        return None

    def register_ticket(self, ticket: Ticket) -> None:
        with self._tickets_lock:
            self._active_tickets[ticket.ticket_id] = ticket

    def release_ticket(self, ticket_id: str) -> Optional[Ticket]:
        with self._tickets_lock:
            ticket = self._active_tickets.pop(ticket_id, None)
        if ticket is None:
            return None
        ticket.spot.unpark()
        self.update_display()
        return ticket

    def get_available_spot_count(self, spot_type: SpotType) -> int:
        return sum(level.available_count(spot_type) for level in self._levels)

    def update_display(self) -> None:
        parts = [f"{t.name}: {self.get_available_spot_count(t)}" for t in SpotType]
        data = "Availability -> " + "  ".join(parts)
        for device in self._display_devices:
            device.update(data)


class EntryGate:
    def __init__(self, manager: ParkingLotManager, ticket_generator: TicketGenerator):
        self._manager = manager
        self._ticket_generator = ticket_generator

    def issue_ticket(self, vehicle: Vehicle) -> Optional[Ticket]:
        spot = self._manager.park_vehicle(vehicle)
        if spot is None:
            print(f"No spot available for {vehicle.get_licence_plate()}")
            return None
        ticket = Ticket(self._ticket_generator.next_ticket_id(), vehicle, spot, time.time())
        self._manager.register_ticket(ticket)
        print(f"Issued {ticket.ticket_id} to {vehicle.get_licence_plate()} at spot {spot.spot_id}")
        return ticket


class ExitGate:
    def __init__(self, manager: ParkingLotManager, payment_strategy: PaymentStrategy):
        self._manager = manager
        self._payment_strategy = payment_strategy

    def process_exit(self, ticket_id: str) -> bool:
        ticket = self._manager.release_ticket(ticket_id)
        if ticket is None:
            print(f"Unknown ticket {ticket_id}")
            return False
        self._payment_strategy.calculate_and_charge(ticket, time.time())
        return True


def _build_levels(num_levels: int, spots_per_type: int) -> List[ParkingLevel]:
    levels = []
    for lvl in range(1, num_levels + 1):
        spots = []
        for spot_type in SpotType:
            for i in range(1, spots_per_type + 1):
                spots.append(ParkingSpot(f"L{lvl}-{spot_type.name}-{i}", lvl, spot_type))
        levels.append(ParkingLevel(lvl, spots))
    return levels


def main() -> None:
    levels = _build_levels(2, 2)  # 2 levels x 2 spots per type = 12 spots total
    manager = ParkingLotManager(levels, BestFitStrategy())
    manager.add_display_device(Screen("EntranceDisplay"))

    ticket_generator = TicketGenerator()
    entry_gate = EntryGate(manager, ticket_generator)
    payment = MembershipDiscountDecorator(HourlyRatePayment(), 10)
    exit_gate = ExitGate(manager, payment)

    incoming = [
        Motorcycle("MC-1"), Car("CAR-1"), Car("CAR-2"),
        Truck("TRK-1"), Motorcycle("MC-2"), Car("CAR-3"),
    ]

    issued_tickets: List[Ticket] = []
    issued_lock = threading.Lock()

    def enter(vehicle: Vehicle) -> None:
        ticket = entry_gate.issue_ticket(vehicle)
        if ticket is not None:
            with issued_lock:
                issued_tickets.append(ticket)

    # Simulate multiple vehicles arriving concurrently from different threads.
    with ThreadPoolExecutor(max_workers=4) as entry_pool:
        futures = [entry_pool.submit(enter, vehicle) for vehicle in incoming]
        for future in futures:
            future.result()

    manager.update_display()

    # A couple of vehicles leave.
    if issued_tickets:
        exit_gate.process_exit(issued_tickets[0].ticket_id)
    if len(issued_tickets) > 1:
        exit_gate.process_exit(issued_tickets[1].ticket_id)

    manager.update_display()
    print(f"Tickets currently active: {len(issued_tickets) - 2}")


if __name__ == "__main__":
    main()
