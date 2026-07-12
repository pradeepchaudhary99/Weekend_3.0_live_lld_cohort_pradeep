/*
 Elevator System
 // vending machine
 // LRU Cache
*/

/*

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
        addInternalRequest()
    Request
    SchedulingStrategy
        Concrete Strategies
        NearestElevatorStrategy
    Direction, ElevatorState, DoorState

*/

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

// ---------- Enums ----------

enum class Direction { UP, DOWN, IDLE };

enum class ElevatorState { IDLE, MOVING, STOPPED };

enum class DoorState { OPEN, CLOSED };

// ---------- Request (hall call value object) ----------

class Request {
public:
    Request(int floor, Direction direction) : floor_(floor), direction_(direction) {}

    int getFloor() const { return floor_; }
    Direction getDirection() const { return direction_; }

private:
    int floor_;
    Direction direction_;
};

// ---------- Elevator: now runs on its own thread ----------

class Elevator {
public:
    Elevator(int id, int capacity, int tickDurationMillis)
        : id_(id),
          capacity_(capacity),
          tickDurationMillis_(tickDurationMillis),
          currentFloor_(0),
          direction_(Direction::IDLE),
          state_(ElevatorState::IDLE),
          doorState_(DoorState::CLOSED),
          running_(false) {}

    int getId() const { return id_; }

    int getCurrentFloor() {
        std::lock_guard<std::mutex> lock(mutex_);
        return currentFloor_;
    }

    Direction getDirection() {
        std::lock_guard<std::mutex> lock(mutex_);
        return direction_;
    }

    ElevatorState getState() {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_;
    }

    // Called from the controller thread while this elevator's own thread may be mid-step()
    void addStop(int floor) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (floor == currentFloor_) {
            return;
        }
        if (floor > currentFloor_) {
            upStops_.insert(floor);
        } else {
            downStops_.insert(floor);
        }
        if (direction_ == Direction::IDLE) {
            direction_ = floor > currentFloor_ ? Direction::UP : Direction::DOWN;
            state_ = ElevatorState::MOVING;
        }
    }

    int distanceTo(int floor) {
        std::lock_guard<std::mutex> lock(mutex_);
        return std::abs(currentFloor_ - floor);
    }

    // Signals this elevator's thread to stop after its current tick
    void shutdown() { running_ = false; }

    // Entry point run on this elevator's dedicated std::thread
    void run() {
        running_ = true;
        std::cout << "Elevator-" << id_ << "-Thread started for Elevator " << id_ << std::endl;
        while (running_) {
            step();
            std::this_thread::sleep_for(std::chrono::milliseconds(tickDurationMillis_));
        }
        std::cout << "Elevator-" << id_ << "-Thread stopped for Elevator " << id_ << std::endl;
    }

private:
    // Whole step is lock-guarded: it's the atomic unit of "this elevator moved one tick"
    void step() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (doorState_ == DoorState::OPEN) {
            closeDoor();
            return;
        }

        if (direction_ == Direction::UP) {
            handleUpStep();
        } else if (direction_ == Direction::DOWN) {
            handleDownStep();
        }
        // if IDLE, nothing pending -> elevator simply waits for the next tick
    }

    void handleUpStep() {
        if (!upStops_.empty()) {
            int target = *upStops_.begin();
            moveTowards(target);
            if (currentFloor_ == target) {
                upStops_.erase(target);
                openDoor();
            }
        } else if (!downStops_.empty()) {
            direction_ = Direction::DOWN;
        } else {
            direction_ = Direction::IDLE;
            state_ = ElevatorState::IDLE;
        }
    }

    void handleDownStep() {
        if (!downStops_.empty()) {
            int target = *downStops_.begin();
            moveTowards(target);
            if (currentFloor_ == target) {
                downStops_.erase(target);
                openDoor();
            }
        } else if (!upStops_.empty()) {
            direction_ = Direction::UP;
        } else {
            direction_ = Direction::IDLE;
            state_ = ElevatorState::IDLE;
        }
    }

    void moveTowards(int target) {
        state_ = ElevatorState::MOVING;
        if (currentFloor_ < target) {
            currentFloor_++;
        } else if (currentFloor_ > target) {
            currentFloor_--;
        }
    }

    void openDoor() {
        doorState_ = DoorState::OPEN;
        state_ = ElevatorState::STOPPED;
        std::cout << "Elevator " << id_ << " -> door OPEN at floor " << currentFloor_ << std::endl;
    }

    void closeDoor() {
        doorState_ = DoorState::CLOSED;
        std::cout << "Elevator " << id_ << " -> door CLOSED at floor " << currentFloor_ << std::endl;
    }

    int id_;
    int capacity_;
    int tickDurationMillis_;

    int currentFloor_;
    Direction direction_;
    ElevatorState state_;
    DoorState doorState_;

    std::set<int> upStops_;                          // ascending: nearest-up first
    std::set<int, std::greater<int>> downStops_;      // descending: nearest-down first

    std::atomic<bool> running_;

    // Guards all mutable state above, mirroring Java's `synchronized` on `this`.
    std::mutex mutex_;
};

// ---------- Scheduling Strategy (Strategy Pattern) ----------

struct SchedulingStrategy {
    virtual ~SchedulingStrategy() = default;
    virtual Elevator* selectElevator(std::vector<std::shared_ptr<Elevator>>& elevators,
                                      const Request& request) = 0;
};

class NearestElevatorStrategy : public SchedulingStrategy {
public:
    Elevator* selectElevator(std::vector<std::shared_ptr<Elevator>>& elevators,
                              const Request& request) override {
        Elevator* best = nullptr;
        int bestDistance = std::numeric_limits<int>::max();

        for (auto& elevator : elevators) {
            bool suitable = elevator->getDirection() == Direction::IDLE ||
                             elevator->getDirection() == request.getDirection();
            int distance = elevator->distanceTo(request.getFloor());
            if (suitable && distance < bestDistance) {
                bestDistance = distance;
                best = elevator.get();
            }
        }

        if (best == nullptr) {
            for (auto& elevator : elevators) {
                int distance = elevator->distanceTo(request.getFloor());
                if (distance < bestDistance) {
                    bestDistance = distance;
                    best = elevator.get();
                }
            }
        }
        return best;
    }
};

// ---------- Elevator Controller: owns thread lifecycle for all elevators ----------

class ElevatorController {
public:
    ElevatorController(std::vector<std::shared_ptr<Elevator>> elevators,
                        std::unique_ptr<SchedulingStrategy> strategy)
        : elevators_(std::move(elevators)), strategy_(std::move(strategy)) {}

    // Spins up one dedicated thread per elevator
    void start() {
        for (auto& elevator : elevators_) {
            elevatorThreads_.emplace_back([elevator]() { elevator->run(); });
        }
    }

    // Lock-guarded: dispatch decisions must be serialized so two concurrent hall
    // calls don't both pick the same elevator off a stale snapshot of its state.
    void requestElevator(int floor, Direction direction) {
        std::lock_guard<std::mutex> lock(mutex_);
        Request request(floor, direction);
        Elevator* chosen = strategy_->selectElevator(elevators_, request);
        if (chosen != nullptr) {
            std::cout << "dispatching Elevator " << chosen->getId() << " to floor " << floor
                      << std::endl;
            chosen->addStop(floor);
        } else {
            std::cout << "No elevator available for floor " << floor << std::endl;
        }
    }

    // Cabin call doesn't need controller-level locking: it targets one specific
    // elevator directly, and that elevator's own addStop() is already lock-guarded.
    void selectFloor(int elevatorId, int destinationFloor) {
        for (auto& elevator : elevators_) {
            if (elevator->getId() == elevatorId) {
                elevator->addStop(destinationFloor);
                return;
            }
        }
    }

    // Signals every elevator to stop, then waits for all their threads to finish
    void shutdown() {
        for (auto& elevator : elevators_) {
            elevator->shutdown();
        }
        for (auto& thread : elevatorThreads_) {
            thread.join();
        }
    }

    std::vector<std::shared_ptr<Elevator>>& getElevators() { return elevators_; }

private:
    std::vector<std::shared_ptr<Elevator>> elevators_;
    std::vector<std::thread> elevatorThreads_;
    std::unique_ptr<SchedulingStrategy> strategy_;
    std::mutex mutex_;
};

// ---------- Building (top-level aggregate) ----------

class Building {
public:
    Building(int numFloors, ElevatorController& controller)
        : numFloors_(numFloors), controller_(controller) {}

    ElevatorController& getController() { return controller_; }
    int getNumFloors() const { return numFloors_; }

private:
    int numFloors_;
    ElevatorController& controller_;
};

// ---------- Demo / Simulation ----------

int main() {
    std::vector<std::shared_ptr<Elevator>> elevators;
    elevators.push_back(std::make_shared<Elevator>(1, 8, 500));
    elevators.push_back(std::make_shared<Elevator>(2, 8, 500));

    auto strategy = std::make_unique<NearestElevatorStrategy>();
    ElevatorController controller(elevators, std::move(strategy));
    Building building(10, controller);

    std::cout << "Starting building with " << elevators.size()
              << " elevators, each on its own thread.\n" << std::endl;
    controller.start();

    // Simulate multiple users concurrently pressing hall/cabin buttons from
    // different threads, hitting the controller at roughly the same time.
    std::vector<std::thread> userSimulator;
    userSimulator.emplace_back([&controller]() { controller.requestElevator(5, Direction::UP); });
    userSimulator.emplace_back([&controller]() { controller.requestElevator(3, Direction::DOWN); });
    userSimulator.emplace_back([&controller]() { controller.selectFloor(1, 8); });
    userSimulator.emplace_back([&controller]() { controller.selectFloor(2, 0); });

    for (auto& thread : userSimulator) {
        thread.join();
    }

    // Let elevators actually run for a while (real time, since each has its own thread)
    std::this_thread::sleep_for(std::chrono::seconds(6));

    controller.shutdown();

    std::cout << "\nFinal elevator positions:" << std::endl;
    for (auto& elevator : controller.getElevators()) {
        std::cout << "Elevator " << elevator->getId() << " at floor "
                  << elevator->getCurrentFloor() << ", state=";
        switch (elevator->getState()) {
            case ElevatorState::IDLE: std::cout << "IDLE"; break;
            case ElevatorState::MOVING: std::cout << "MOVING"; break;
            case ElevatorState::STOPPED: std::cout << "STOPPED"; break;
        }
        std::cout << std::endl;
    }

    return 0;
}
