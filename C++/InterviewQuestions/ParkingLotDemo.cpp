/*
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
*/

#include <atomic>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

enum class VehicleType { MOTORCYCLE, CAR, TRUCK };

enum class SpotType { SMALL, MEDIUM, LARGE };

std::string spotTypeName(SpotType type) {
    switch (type) {
        case SpotType::SMALL: return "SMALL";
        case SpotType::MEDIUM: return "MEDIUM";
        case SpotType::LARGE: return "LARGE";
    }
    return "UNKNOWN";
}

// ---------- Vehicle hierarchy ----------

class Vehicle {
public:
    Vehicle(std::string licencePlate, VehicleType type)
        : licencePlate_(std::move(licencePlate)), type_(type) {}
    virtual ~Vehicle() = default;

    const std::string& getLicencePlate() const { return licencePlate_; }
    VehicleType getType() const { return type_; }

    virtual bool canFitIn(SpotType spotType) const = 0;

protected:
    std::string licencePlate_;
    VehicleType type_;
};

class Motorcycle : public Vehicle {
public:
    explicit Motorcycle(std::string licencePlate) : Vehicle(std::move(licencePlate), VehicleType::MOTORCYCLE) {}
    bool canFitIn(SpotType) const override { return true; }
};

class Car : public Vehicle {
public:
    explicit Car(std::string licencePlate) : Vehicle(std::move(licencePlate), VehicleType::CAR) {}
    bool canFitIn(SpotType spotType) const override {
        return spotType == SpotType::MEDIUM || spotType == SpotType::LARGE;
    }
};

class Truck : public Vehicle {
public:
    explicit Truck(std::string licencePlate) : Vehicle(std::move(licencePlate), VehicleType::TRUCK) {}
    bool canFitIn(SpotType spotType) const override { return spotType == SpotType::LARGE; }
};

// ---------- Parking spot ----------

// Occupancy is claimed with tryPark(), guarded by a per-spot mutex so two
// threads racing for the same spot can never both succeed.
class ParkingSpot {
public:
    ParkingSpot(std::string spotId, int levelNumber, SpotType spotType)
        : spotId_(std::move(spotId)), levelNumber_(levelNumber), spotType_(spotType) {}

    const std::string& getSpotId() const { return spotId_; }
    int getLevelNumber() const { return levelNumber_; }
    SpotType getSpotType() const { return spotType_; }

    bool isOccupied() {
        std::lock_guard<std::mutex> lock(mutex_);
        return vehicle_ != nullptr;
    }

    bool tryPark(const std::shared_ptr<Vehicle>& candidate) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (vehicle_ != nullptr) {
            return false;
        }
        vehicle_ = candidate;
        return true;
    }

    std::shared_ptr<Vehicle> unpark() {
        std::lock_guard<std::mutex> lock(mutex_);
        auto parked = vehicle_;
        vehicle_ = nullptr;
        return parked;
    }

private:
    std::string spotId_;
    int levelNumber_;
    SpotType spotType_;
    std::shared_ptr<Vehicle> vehicle_;
    std::mutex mutex_;
};

// ---------- Parking level ----------

class ParkingLevel {
public:
    ParkingLevel(int levelNumber, std::vector<std::shared_ptr<ParkingSpot>> spots)
        : levelNumber_(levelNumber), spots_(std::move(spots)) {}

    int getLevelNumber() const { return levelNumber_; }
    const std::vector<std::shared_ptr<ParkingSpot>>& getSpots() const { return spots_; }

    long availableCount(SpotType spotType) {
        long count = 0;
        for (auto& spot : spots_) {
            if (spot->getSpotType() == spotType && !spot->isOccupied()) {
                ++count;
            }
        }
        return count;
    }

private:
    int levelNumber_;
    std::vector<std::shared_ptr<ParkingSpot>> spots_;
};

// ---------- Spot assignment strategy (Strategy pattern) ----------

struct SpotAssignmentStrategy {
    virtual ~SpotAssignmentStrategy() = default;
    // Returns candidate spots in the order they should be attempted; the
    // caller races tryPark() over them so a losing candidate just moves on.
    virtual std::vector<std::shared_ptr<ParkingSpot>> candidateSpots(
        const std::vector<std::shared_ptr<ParkingLevel>>& levels, const std::shared_ptr<Vehicle>& vehicle) = 0;
};

class FirstFitStrategy : public SpotAssignmentStrategy {
public:
    std::vector<std::shared_ptr<ParkingSpot>> candidateSpots(
        const std::vector<std::shared_ptr<ParkingLevel>>& levels, const std::shared_ptr<Vehicle>& vehicle) override {
        std::vector<std::shared_ptr<ParkingSpot>> candidates;
        for (auto& level : levels) {
            for (auto& spot : level->getSpots()) {
                if (vehicle->canFitIn(spot->getSpotType())) {
                    candidates.push_back(spot);
                }
            }
        }
        return candidates;
    }
};

// Prefers the smallest spot type the vehicle fits in, to save larger spots
// for vehicles that actually need them.
class BestFitStrategy : public SpotAssignmentStrategy {
public:
    std::vector<std::shared_ptr<ParkingSpot>> candidateSpots(
        const std::vector<std::shared_ptr<ParkingLevel>>& levels, const std::shared_ptr<Vehicle>& vehicle) override {
        static const std::vector<SpotType> sizeOrder = {SpotType::SMALL, SpotType::MEDIUM, SpotType::LARGE};
        std::vector<std::shared_ptr<ParkingSpot>> candidates;
        for (SpotType type : sizeOrder) {
            if (!vehicle->canFitIn(type)) {
                continue;
            }
            for (auto& level : levels) {
                for (auto& spot : level->getSpots()) {
                    if (spot->getSpotType() == type) {
                        candidates.push_back(spot);
                    }
                }
            }
        }
        return candidates;
    }
};

// ---------- Ticket ----------

class Ticket {
public:
    Ticket(std::string ticketId, std::shared_ptr<Vehicle> vehicle, std::shared_ptr<ParkingSpot> spot,
           std::chrono::steady_clock::time_point entryTime)
        : ticketId_(std::move(ticketId)), vehicle_(std::move(vehicle)), spot_(std::move(spot)), entryTime_(entryTime) {}

    const std::string& getTicketId() const { return ticketId_; }
    const std::shared_ptr<Vehicle>& getVehicle() const { return vehicle_; }
    const std::shared_ptr<ParkingSpot>& getSpot() const { return spot_; }
    std::chrono::steady_clock::time_point getEntryTime() const { return entryTime_; }

private:
    std::string ticketId_;
    std::shared_ptr<Vehicle> vehicle_;
    std::shared_ptr<ParkingSpot> spot_;
    std::chrono::steady_clock::time_point entryTime_;
};

class TicketGenerator {
public:
    std::string nextTicketId() { return "T-" + std::to_string(++sequence_); }

private:
    std::atomic<long long> sequence_{0};
};

// ---------- Payment (Strategy pattern, decorated) ----------

struct PaymentStrategy {
    virtual ~PaymentStrategy() = default;
    virtual double calculateAndCharge(const std::shared_ptr<Ticket>& ticket,
                                       std::chrono::steady_clock::time_point exitTime) = 0;
};

class HourlyRatePayment : public PaymentStrategy {
public:
    double calculateAndCharge(const std::shared_ptr<Ticket>& ticket,
                               std::chrono::steady_clock::time_point exitTime) override {
        double durationSeconds =
            std::chrono::duration<double>(exitTime - ticket->getEntryTime()).count();
        double hours = std::max(1.0, durationSeconds / 3600.0); // minimum 1 hour billed
        double amount = hours * kRatePerHour;
        std::printf("Charging %.2f for ticket %s (%.2f hours)\n", amount, ticket->getTicketId().c_str(), hours);
        return amount;
    }

private:
    static constexpr double kRatePerHour = 20.0;
};

// Decorator pattern: wraps any PaymentStrategy to apply a membership discount
// without the base strategy knowing anything about memberships.
class PaymentDecorator : public PaymentStrategy {
protected:
    std::shared_ptr<PaymentStrategy> delegate_;
    explicit PaymentDecorator(std::shared_ptr<PaymentStrategy> delegate) : delegate_(std::move(delegate)) {}
};

class MembershipDiscountDecorator : public PaymentDecorator {
public:
    MembershipDiscountDecorator(std::shared_ptr<PaymentStrategy> delegate, double discountPercent)
        : PaymentDecorator(std::move(delegate)), discountPercent_(discountPercent) {}

    double calculateAndCharge(const std::shared_ptr<Ticket>& ticket,
                               std::chrono::steady_clock::time_point exitTime) override {
        double base = delegate_->calculateAndCharge(ticket, exitTime);
        double discounted = base * (1 - discountPercent_ / 100.0);
        std::printf("Applying %.0f%% membership discount -> %.2f\n", discountPercent_, discounted);
        return discounted;
    }

private:
    double discountPercent_;
};

// ---------- Observer pattern: display boards ----------

struct DisplayDevice {
    virtual ~DisplayDevice() = default;
    virtual void update(const std::string& data) = 0;
};

class Screen : public DisplayDevice {
public:
    explicit Screen(std::string name) : name_(std::move(name)) {}
    void update(const std::string& data) override {
        std::cout << "[" << name_ << "] " << data << "\n";
    }

private:
    std::string name_;
};

// ---------- Parking lot manager (orchestrator) ----------

class ParkingLotManager {
public:
    ParkingLotManager(std::vector<std::shared_ptr<ParkingLevel>> levels,
                       std::shared_ptr<SpotAssignmentStrategy> strategy)
        : levels_(std::move(levels)), assignmentStrategy_(std::move(strategy)) {}

    void addDisplayDevice(std::shared_ptr<DisplayDevice> device) {
        std::lock_guard<std::mutex> lock(displayMutex_);
        displayDevices_.push_back(std::move(device));
    }

    // Races candidate spots one at a time: tryPark() is atomic per-spot, so
    // concurrent callers can never double-park the same spot, and no global
    // lock is needed across the whole manager.
    std::shared_ptr<ParkingSpot> parkVehicle(const std::shared_ptr<Vehicle>& vehicle) {
        auto candidates = assignmentStrategy_->candidateSpots(levels_, vehicle);
        for (auto& spot : candidates) {
            if (spot->tryPark(vehicle)) {
                updateDisplay();
                return spot;
            }
        }
        return nullptr;
    }

    void registerTicket(const std::shared_ptr<Ticket>& ticket) {
        std::lock_guard<std::mutex> lock(ticketsMutex_);
        activeTickets_[ticket->getTicketId()] = ticket;
    }

    std::shared_ptr<Ticket> releaseTicket(const std::string& ticketId) {
        std::shared_ptr<Ticket> ticket;
        {
            std::lock_guard<std::mutex> lock(ticketsMutex_);
            auto it = activeTickets_.find(ticketId);
            if (it == activeTickets_.end()) {
                return nullptr;
            }
            ticket = it->second;
            activeTickets_.erase(it);
        }
        ticket->getSpot()->unpark();
        updateDisplay();
        return ticket;
    }

    long getAvailableSpotCount(SpotType spotType) {
        long total = 0;
        for (auto& level : levels_) {
            total += level->availableCount(spotType);
        }
        return total;
    }

    void updateDisplay() {
        std::string data = "Availability -> ";
        for (SpotType type : {SpotType::SMALL, SpotType::MEDIUM, SpotType::LARGE}) {
            data += spotTypeName(type) + ": " + std::to_string(getAvailableSpotCount(type)) + "  ";
        }
        std::lock_guard<std::mutex> lock(displayMutex_);
        for (auto& device : displayDevices_) {
            device->update(data);
        }
    }

private:
    std::vector<std::shared_ptr<ParkingLevel>> levels_;
    std::shared_ptr<SpotAssignmentStrategy> assignmentStrategy_;
    std::vector<std::shared_ptr<DisplayDevice>> displayDevices_;
    std::mutex displayMutex_;
    std::unordered_map<std::string, std::shared_ptr<Ticket>> activeTickets_;
    std::mutex ticketsMutex_;
};

// ---------- Gates ----------

class EntryGate {
public:
    EntryGate(std::shared_ptr<ParkingLotManager> manager, std::shared_ptr<TicketGenerator> ticketGenerator)
        : manager_(std::move(manager)), ticketGenerator_(std::move(ticketGenerator)) {}

    std::shared_ptr<Ticket> issueTicket(const std::shared_ptr<Vehicle>& vehicle) {
        auto spot = manager_->parkVehicle(vehicle);
        if (!spot) {
            std::cout << "No spot available for " << vehicle->getLicencePlate() << "\n";
            return nullptr;
        }
        auto ticket = std::make_shared<Ticket>(ticketGenerator_->nextTicketId(), vehicle, spot,
                                                std::chrono::steady_clock::now());
        manager_->registerTicket(ticket);
        std::cout << "Issued " << ticket->getTicketId() << " to " << vehicle->getLicencePlate()
                  << " at spot " << spot->getSpotId() << "\n";
        return ticket;
    }

private:
    std::shared_ptr<ParkingLotManager> manager_;
    std::shared_ptr<TicketGenerator> ticketGenerator_;
};

class ExitGate {
public:
    ExitGate(std::shared_ptr<ParkingLotManager> manager, std::shared_ptr<PaymentStrategy> paymentStrategy)
        : manager_(std::move(manager)), paymentStrategy_(std::move(paymentStrategy)) {}

    bool processExit(const std::string& ticketId) {
        auto ticket = manager_->releaseTicket(ticketId);
        if (!ticket) {
            std::cout << "Unknown ticket " << ticketId << "\n";
            return false;
        }
        paymentStrategy_->calculateAndCharge(ticket, std::chrono::steady_clock::now());
        return true;
    }

private:
    std::shared_ptr<ParkingLotManager> manager_;
    std::shared_ptr<PaymentStrategy> paymentStrategy_;
};

// ---------- Demo / Simulation ----------

std::vector<std::shared_ptr<ParkingLevel>> buildLevels(int numLevels, int spotsPerType) {
    std::vector<std::shared_ptr<ParkingLevel>> levels;
    for (int lvl = 1; lvl <= numLevels; ++lvl) {
        std::vector<std::shared_ptr<ParkingSpot>> spots;
        for (SpotType type : {SpotType::SMALL, SpotType::MEDIUM, SpotType::LARGE}) {
            for (int i = 1; i <= spotsPerType; ++i) {
                spots.push_back(std::make_shared<ParkingSpot>(
                    "L" + std::to_string(lvl) + "-" + spotTypeName(type) + "-" + std::to_string(i), lvl, type));
            }
        }
        levels.push_back(std::make_shared<ParkingLevel>(lvl, std::move(spots)));
    }
    return levels;
}

int main() {
    auto levels = buildLevels(2, 2); // 2 levels x 2 spots per type = 12 spots total
    auto manager = std::make_shared<ParkingLotManager>(levels, std::make_shared<BestFitStrategy>());
    manager->addDisplayDevice(std::make_shared<Screen>("EntranceDisplay"));

    auto ticketGenerator = std::make_shared<TicketGenerator>();
    EntryGate entryGate(manager, ticketGenerator);
    auto payment = std::make_shared<MembershipDiscountDecorator>(std::make_shared<HourlyRatePayment>(), 10);
    ExitGate exitGate(manager, payment);

    std::vector<std::shared_ptr<Vehicle>> incoming = {
        std::make_shared<Motorcycle>("MC-1"), std::make_shared<Car>("CAR-1"), std::make_shared<Car>("CAR-2"),
        std::make_shared<Truck>("TRK-1"), std::make_shared<Motorcycle>("MC-2"), std::make_shared<Car>("CAR-3")};

    std::vector<std::shared_ptr<Ticket>> issuedTickets;
    std::mutex issuedMutex;

    // Simulate multiple vehicles arriving concurrently from different threads.
    std::vector<std::thread> entryThreads;
    for (auto& vehicle : incoming) {
        entryThreads.emplace_back([&entryGate, &issuedTickets, &issuedMutex, vehicle]() {
            auto ticket = entryGate.issueTicket(vehicle);
            if (ticket) {
                std::lock_guard<std::mutex> lock(issuedMutex);
                issuedTickets.push_back(ticket);
            }
        });
    }
    for (auto& t : entryThreads) {
        t.join();
    }

    manager->updateDisplay();

    // A couple of vehicles leave.
    if (!issuedTickets.empty()) {
        exitGate.processExit(issuedTickets[0]->getTicketId());
    }
    if (issuedTickets.size() > 1) {
        exitGate.processExit(issuedTickets[1]->getTicketId());
    }

    manager->updateDisplay();
    std::cout << "Tickets currently active: " << (static_cast<int>(issuedTickets.size()) - 2) << "\n";

    return 0;
}
