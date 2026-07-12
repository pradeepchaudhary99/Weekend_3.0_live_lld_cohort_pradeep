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
//Relationship part:

"""

from abc import ABC, abstractmethod
from enum import Enum, auto
from typing import Dict, List, Optional


class VehicleType(Enum):
    MOTORCYCLE = auto()
    CAR = auto()
    TRUCK = auto()


class SpotType(Enum):
    LARGE = auto()
    MEDIUM = auto()
    SMALL = auto()


class Vehicle(ABC):
    def __init__(self, licence_plate: str, vehicle_type: VehicleType):
        self._licence_plate = licence_plate
        self._type = vehicle_type

    def get_licence_plate(self) -> str:
        return self._licence_plate

    def get_type(self) -> VehicleType:
        return self._type


class ParkingSpot:
    def __init__(self, spot_type: SpotType):
        self.spot_type = spot_type
        self.vehicle: Optional[Vehicle] = None

    def park(self, vehicle: Vehicle) -> None:
        pass

    def unpark(self) -> None:
        pass


class ParkingLevel:
    def __init__(self):
        self.spots: List[ParkingSpot] = []

    def park(self, vehicle: Vehicle) -> bool:
        pass

    def unpark(self, vehicle_id: str) -> None:
        pass


class SpotAssignmentStrategy(ABC):
    @abstractmethod
    def find_spot(self, levels: List["ParkingLevel"], vehicle_type: VehicleType) -> Optional[ParkingSpot]:
        raise NotImplementedError


class FirstFitStrategy(SpotAssignmentStrategy):
    def find_spot(self, levels: List["ParkingLevel"], vehicle_type: VehicleType) -> Optional[ParkingSpot]:
        pass


class Ticket:
    def __init__(self, spot: ParkingSpot):
        self.spot = spot


class EntryGate:
    pass


class ExitGate:
    def __init__(self, payment_strategy=None):
        self.payment = payment_strategy


class DisplayDevice(ABC):
    @abstractmethod
    def update(self, data: str) -> None:
        raise NotImplementedError


class Screen(DisplayDevice):
    def update(self, data: str) -> None:
        print("Updates")


class ParkingLotManager:
    def __init__(self, assignment_strategy: SpotAssignmentStrategy):
        self.all_devices: List[DisplayDevice] = []  # observers
        self.levels: List[ParkingLevel] = []
        self.assignment_strategy = assignment_strategy
        self.active_tickets: Dict[str, Ticket] = {}

    def park_vehicle(self, vehicle: Vehicle) -> bool:
        pass

    def unpark_vehicle(self, vehicle_id: str) -> None:
        pass

    def get_available_spot_count(self, spot_type: SpotType) -> int:
        pass

    def update_display(self) -> None:
        pass


def main() -> None:
    pass


if __name__ == "__main__":
    main()
