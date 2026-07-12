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
//Relationship part:

*/

#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

enum class VehicleType { MOTORCYCLE, CAR, TRUCK };

enum class SpotType { LARGE, MEDIUM, SMALL };

class Vehicle {
public:
    Vehicle(std::string licencePlate, VehicleType type)
        : licencePlate_(std::move(licencePlate)), type_(type) {}
    virtual ~Vehicle() = default;

    const std::string& getLicencePlate() const { return licencePlate_; }
    VehicleType getType() const { return type_; }

protected:
    std::string licencePlate_;
    VehicleType type_;
};

class ParkingSpot {
public:
    explicit ParkingSpot(SpotType spotType) : spotType_(spotType) {}

    void park(const std::shared_ptr<Vehicle>& vehicle) { vehicle_ = vehicle; }
    void unpark() { vehicle_.reset(); }

    SpotType spotType_;
    std::shared_ptr<Vehicle> vehicle_;
};

class ParkingLevel {
public:
    bool park(const std::shared_ptr<Vehicle>& vehicle) { return false; }
    void unpark(const std::string& vehicleId) {}

    std::vector<std::unique_ptr<ParkingSpot>> spots_;
};

struct SpotAssignmentStrategy {
    virtual ~SpotAssignmentStrategy() = default;
    virtual ParkingSpot* findSpot(std::vector<std::unique_ptr<ParkingLevel>>& levels,
                                   VehicleType vehicleType) = 0;
};

class FirstFitStrategy : public SpotAssignmentStrategy {
public:
    ParkingSpot* findSpot(std::vector<std::unique_ptr<ParkingLevel>>& levels,
                           VehicleType vehicleType) override {
        return nullptr;
    }
};

class Ticket {
public:
    explicit Ticket(ParkingSpot* spot) : spot_(spot) {}
    ParkingSpot* spot_;
};

class EntryGate {};

class ExitGate {
public:
    void* payment_ = nullptr;  // PaymentStrategy placeholder
};

struct DisplayDevice {
    virtual ~DisplayDevice() = default;
    virtual void update(const std::string& data) = 0;
};

class Screen : public DisplayDevice {
public:
    void update(const std::string& data) override {
        std::cout << "Updates" << std::endl;
    }
};

class ParkingLotManager {
public:
    explicit ParkingLotManager(std::unique_ptr<SpotAssignmentStrategy> strategy)
        : assignmentStrategy_(std::move(strategy)) {}

    bool parkVehicle(const std::shared_ptr<Vehicle>& vehicle) { return false; }
    void unparkVehicle(const std::string& vehicleId) {}
    int getAvailableSpotCount(SpotType spotType) const { return 0; }
    void updateDisplay() {}

private:
    std::vector<std::shared_ptr<DisplayDevice>> allDevices_;  // observers
    std::vector<std::unique_ptr<ParkingLevel>> levels_;
    std::unique_ptr<SpotAssignmentStrategy> assignmentStrategy_;
    std::unordered_map<std::string, std::unique_ptr<Ticket>> activeTickets_;
};

int main() {
    return 0;
}
