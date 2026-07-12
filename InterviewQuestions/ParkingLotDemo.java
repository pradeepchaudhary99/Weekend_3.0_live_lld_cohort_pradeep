


/*
FR:

The parkinglot has multiple floors/levels,each level with fixed/variable number of spots/slots
Spots should support multiple types of vechicle : MOTORCYCLE, LARGE, COMPACT, HANDICAPPED

Different Vehicle types are allowed: MOTORCYCLE, CAR, TRUCK
System Should generate ticket at the time of Entry Gate 
System Should assign one Slot at some level for the vehicle 
System should have multiple strategies for spot selection/assignment 
System should support multiple payment methods at exit gate 

parkingLot should display the availability at each level
---- Add some decorator Design pattern use case ------


NFR: 
 Thread-safety
 Extensibility   : new Vechicle types, spots types, assignment strategies....


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

enum VehicleType{
    MOTORCYCLE, CAR, TRUCK
}

enum SpotType{
    LARGE, MEDIUM, SMALL
}

abstract class Vehicle{
    - String licencePlate 
    - VehicleType type 
    +getLincensePlate() 
    +getType()
}

class parkingSpot{
    SpotType
    Vechicle
    
    park()
    unpark()

}


class ParkingLevel{
    List<ParkingSpot>
    
    Park() 
    unpark()
}


interface SpotAssignmentStrategy{
    +findSpot(List<ParkingLevel>, VehicleType);
}

class FirstFitStrategy implements SpotAssignmentStrategy{

}

class Ticket{
    ParkingSpot
}

class EntryGate{

}

class ExitGate{
    PaymentStrategy payment;
}

class ParkingLotManager{

    List<DisplayDevices> allDevices; //observers 

    List<parkingLevel> levels
    SpotAssignmentStrategy assignmentStrategy
    ConcurrentHashMap<String, Ticket> activeTickets 

    parkVehicle(Vehicle) : boolean 
    unparkVehicle(String vehicleId)
    
    +getAvailableSpotCount(spotType) 

    void updateDisplay(){

    }

}

interface DisplayDevices{
    void update(String data);
}

class Screen implements DisplayDevices{
    public void update(String data){
        System.out.println("Updates");
    }
}


public class ParkingLotDemo {
    
}
