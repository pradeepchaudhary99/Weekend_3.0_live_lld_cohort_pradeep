"""
Elevator System
// vending machine
// LRU Cache
"""

"""
Elevator System
------------------------------

Elevator System
Functional Requirements:
    The System should be able to support N number of elevator
    A user should be able to make a hall call - Press UP or DOWN
    A user inside an elevator can make a cabin call - select a destination floor
    The should support multiple algorithms for elevator selection
    Each elevator has a direction(UP/DOWN/IDLE) and states (MOVING, STOPPED, IDLE)
    The system should have an ElevatorController managing request and assigning elevator
    ----- Good To have ----- OUT of SCOPE ---
    Capacity Overload handling
    DisplayBoard on each floor
    Emergency handling

Non-Functional:
    Scalability - adding elevators shouldn't require design changes
    Extensibility
    Thread-safety
        --> Each elevator is running in it's own thread
        --> multiple users should be able make the calls
        --> maintain the consistency in between multiple calls
    Clean and SOLID principle adhering code...

Core Entities:
    Elevator --->
        add_internal_request()
    Request
    SchedulingStrategy
        Concrete Strategies
        NearestElevatorStrategy
    Direction, ElevatorState, DoorState

"""

import threading
import time
from abc import ABC, abstractmethod
from concurrent.futures import ThreadPoolExecutor
from enum import Enum, auto
from typing import List, Optional


# ---------- Enums ----------


class Direction(Enum):
    UP = auto()
    DOWN = auto()
    IDLE = auto()


class ElevatorState(Enum):
    IDLE = auto()
    MOVING = auto()
    STOPPED = auto()


class DoorState(Enum):
    OPEN = auto()
    CLOSED = auto()


# ---------- Request (hall call value object) ----------


class Request:
    def __init__(self, floor: int, direction: Direction):
        self._floor = floor
        self._direction = direction

    def get_floor(self) -> int:
        return self._floor

    def get_direction(self) -> Direction:
        return self._direction


# ---------- Elevator: now runs on its own thread ----------


class Elevator:
    def __init__(self, elevator_id: int, capacity: int, tick_duration_seconds: float):
        self._id = elevator_id
        self._capacity = capacity
        self._tick_duration_seconds = tick_duration_seconds

        self._current_floor = 0
        self._direction = Direction.IDLE
        self._state = ElevatorState.IDLE
        self._door_state = DoorState.CLOSED

        self._up_stops: set = set()
        self._down_stops: set = set()

        # Flipped from the controller thread to stop this elevator's loop.
        self._running = False

        # Guards all mutable state above, mirroring Java's `synchronized` on `this`.
        self._lock = threading.RLock()

    def get_id(self) -> int:
        return self._id

    def get_current_floor(self) -> int:
        with self._lock:
            return self._current_floor

    def get_direction(self) -> Direction:
        with self._lock:
            return self._direction

    def get_state(self) -> ElevatorState:
        with self._lock:
            return self._state

    # Called from the controller thread while this elevator's own thread may be mid-_step()
    def add_stop(self, floor: int) -> None:
        with self._lock:
            if floor == self._current_floor:
                return
            if floor > self._current_floor:
                self._up_stops.add(floor)
            else:
                self._down_stops.add(floor)
            if self._direction == Direction.IDLE:
                self._direction = Direction.UP if floor > self._current_floor else Direction.DOWN
                self._state = ElevatorState.MOVING

    def distance_to(self, floor: int) -> int:
        with self._lock:
            return abs(self._current_floor - floor)

    # Signals this elevator's thread to stop after its current tick
    def shutdown(self) -> None:
        self._running = False

    def run(self) -> None:
        self._running = True
        thread_name = threading.current_thread().name
        print(f"{thread_name} started for Elevator {self._id}")
        while self._running:
            self._step()
            time.sleep(self._tick_duration_seconds)
        print(f"{thread_name} stopped for Elevator {self._id}")

    # Whole step is lock-guarded: it's the atomic unit of "this elevator moved one tick"
    def _step(self) -> None:
        with self._lock:
            if self._door_state == DoorState.OPEN:
                self._close_door()
                return

            if self._direction == Direction.UP:
                self._handle_up_step()
            elif self._direction == Direction.DOWN:
                self._handle_down_step()
            # if IDLE, nothing pending -> elevator simply waits for the next tick

    def _handle_up_step(self) -> None:
        if self._up_stops:
            target = min(self._up_stops)
            self._move_towards(target)
            if self._current_floor == target:
                self._up_stops.discard(target)
                self._open_door()
        elif self._down_stops:
            self._direction = Direction.DOWN
        else:
            self._direction = Direction.IDLE
            self._state = ElevatorState.IDLE

    def _handle_down_step(self) -> None:
        if self._down_stops:
            target = max(self._down_stops)
            self._move_towards(target)
            if self._current_floor == target:
                self._down_stops.discard(target)
                self._open_door()
        elif self._up_stops:
            self._direction = Direction.UP
        else:
            self._direction = Direction.IDLE
            self._state = ElevatorState.IDLE

    def _move_towards(self, target: int) -> None:
        self._state = ElevatorState.MOVING
        if self._current_floor < target:
            self._current_floor += 1
        elif self._current_floor > target:
            self._current_floor -= 1

    def _open_door(self) -> None:
        self._door_state = DoorState.OPEN
        self._state = ElevatorState.STOPPED
        print(f"Elevator {self._id} -> door OPEN at floor {self._current_floor}")

    def _close_door(self) -> None:
        self._door_state = DoorState.CLOSED
        print(f"Elevator {self._id} -> door CLOSED at floor {self._current_floor}")


# ---------- Scheduling Strategy (Strategy Pattern) ----------


class SchedulingStrategy(ABC):
    @abstractmethod
    def select_elevator(self, elevators: List[Elevator], request: Request) -> Optional[Elevator]:
        raise NotImplementedError


class NearestElevatorStrategy(SchedulingStrategy):
    def select_elevator(self, elevators: List[Elevator], request: Request) -> Optional[Elevator]:
        best: Optional[Elevator] = None
        best_distance = float("inf")

        for elevator in elevators:
            suitable = (
                elevator.get_direction() == Direction.IDLE
                or elevator.get_direction() == request.get_direction()
            )
            distance = elevator.distance_to(request.get_floor())
            if suitable and distance < best_distance:
                best_distance = distance
                best = elevator

        if best is None:
            for elevator in elevators:
                distance = elevator.distance_to(request.get_floor())
                if distance < best_distance:
                    best_distance = distance
                    best = elevator
        return best


# ---------- Elevator Controller: owns thread lifecycle for all elevators ----------


class ElevatorController:
    def __init__(self, elevators: List[Elevator], strategy: SchedulingStrategy):
        self._elevators = elevators
        self._strategy = strategy
        self._elevator_threads: List[threading.Thread] = []
        self._lock = threading.Lock()

    # Spins up one dedicated thread per elevator
    def start(self) -> None:
        for elevator in self._elevators:
            thread = threading.Thread(
                target=elevator.run, name=f"Elevator-{elevator.get_id()}-Thread"
            )
            self._elevator_threads.append(thread)
            thread.start()

    # Lock-guarded: dispatch decisions must be serialized so two concurrent hall
    # calls don't both pick the same elevator off a stale snapshot of its state.
    def request_elevator(self, floor: int, direction: Direction) -> None:
        with self._lock:
            request = Request(floor, direction)
            chosen = self._strategy.select_elevator(self._elevators, request)
            if chosen is not None:
                thread_name = threading.current_thread().name
                print(f"{thread_name} dispatching Elevator {chosen.get_id()} to floor {floor}")
                chosen.add_stop(floor)
            else:
                print(f"No elevator available for floor {floor}")

    # Cabin call doesn't need controller-level locking: it targets one specific
    # elevator directly, and that elevator's own add_stop() is already lock-guarded.
    def select_floor(self, elevator_id: int, destination_floor: int) -> None:
        for elevator in self._elevators:
            if elevator.get_id() == elevator_id:
                elevator.add_stop(destination_floor)
                return

    # Signals every elevator to stop, then waits for all their threads to finish
    def shutdown(self) -> None:
        for elevator in self._elevators:
            elevator.shutdown()
        for thread in self._elevator_threads:
            thread.join()

    def get_elevators(self) -> List[Elevator]:
        return self._elevators


# ---------- Building (top-level aggregate) ----------


class Building:
    def __init__(self, num_floors: int, controller: ElevatorController):
        self._num_floors = num_floors
        self._controller = controller

    def get_controller(self) -> ElevatorController:
        return self._controller

    def get_num_floors(self) -> int:
        return self._num_floors


# ---------- Demo / Simulation ----------


def main() -> None:
    elevators = [Elevator(1, 8, 0.5), Elevator(2, 8, 0.5)]

    strategy = NearestElevatorStrategy()
    controller = ElevatorController(elevators, strategy)
    building = Building(10, controller)

    print(f"Starting building with {len(elevators)} elevators, each on its own thread.\n")
    controller.start()

    # Simulate multiple users concurrently pressing hall/cabin buttons from
    # different threads, hitting the controller at roughly the same time.
    with ThreadPoolExecutor(max_workers=4) as user_simulator:
        user_simulator.submit(controller.request_elevator, 5, Direction.UP)
        user_simulator.submit(controller.request_elevator, 3, Direction.DOWN)
        user_simulator.submit(controller.select_floor, 1, 8)
        user_simulator.submit(controller.select_floor, 2, 0)

    # Let elevators actually run for a while (real time, since each has its own thread)
    time.sleep(6)

    controller.shutdown()

    print("\nFinal elevator positions:")
    for elevator in controller.get_elevators():
        print(
            f"Elevator {elevator.get_id()} at floor {elevator.get_current_floor()}, "
            f"state={elevator.get_state().name}"
        )


if __name__ == "__main__":
    main()
