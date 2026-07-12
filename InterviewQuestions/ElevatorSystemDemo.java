
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
    A user inside an elevator can make a cabin call - slect a destination floor 
    The should support multiple algorithms for elevator selection
    Each elevator has a direction(UP/DOWN/IDLE) and states (MOVING, STOPPED, IDLE)
    The system should have an ElevatorController managing request and assigning elevator
    ----- Good To have ----- OUT of SCOPE ---
    Capacity Overload handling 
    DisplayBoard on each floor 
    Emergency handling 

Non-Functional: 
    Scalability - adding elavtors should'nt require design changes 
    Extensibility
    Thread-safety
        --> Each elevator is running in it's own thread 
        --> multiple users should be able make the calls 
        --> maintain the consistency in between multiple calls
    Clean and SOLID principle adhearing code...

Core Entities: 
    Elevator ---> 
        addInternalRequest()
    Request
    SchedulingStrategy 
        Concrete Stategies 
        NearestElevatorStrategy 
    Direction, ElevatorState, DoorState 

*/

import java.util.*;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

// ---------- Enums ----------

enum Direction {
    UP, DOWN, IDLE
}

enum ElevatorState {
    IDLE, MOVING, STOPPED
}

enum DoorState {
    OPEN, CLOSED
}

// ---------- Request (hall call value object) ----------

class Request {
    private final int floor;
    private final Direction direction;

    public Request(int floor, Direction direction) {
        this.floor = floor;
        this.direction = direction;
    }

    public int getFloor() {
        return floor;
    }

    public Direction getDirection() {
        return direction;
    }
}

// ---------- Elevator: now runs on its own thread ----------

class Elevator implements Runnable {

    private final int id;
    private final int capacity;
    private final int tickDurationMillis;

    private int currentFloor;
    private Direction direction;
    private ElevatorState state;
    private DoorState doorState;

    private final TreeSet<Integer> upStops;
    private final TreeSet<Integer> downStops;

    // Flipped from the controller thread to stop this elevator's loop -> must be volatile
    private volatile boolean running;

    public Elevator(int id, int capacity, int tickDurationMillis) {
        this.id = id;
        this.capacity = capacity;
        this.tickDurationMillis = tickDurationMillis;
        this.currentFloor = 0;
        this.direction = Direction.IDLE;
        this.state = ElevatorState.IDLE;
        this.doorState = DoorState.CLOSED;
        this.upStops = new TreeSet<>();
        this.downStops = new TreeSet<>(Collections.reverseOrder());
        this.running = false;
    }

    public int getId() {
        return id;
    }

    public synchronized int getCurrentFloor() {
        return currentFloor;
    }

    public synchronized Direction getDirection() {
        return direction;
    }

    public synchronized ElevatorState getState() {
        return state;
    }

    // Called from the controller thread while this elevator's own thread may be mid-step()
    public synchronized void addStop(int floor) {
        if (floor == currentFloor) {
            return;
        }
        if (floor > currentFloor) {
            upStops.add(floor);
        } else {
            downStops.add(floor);
        }
        if (direction == Direction.IDLE) {
            direction = floor > currentFloor ? Direction.UP : Direction.DOWN;
            state = ElevatorState.MOVING;
        }
    }

    public synchronized int distanceTo(int floor) {
        return Math.abs(currentFloor - floor);
    }

    // Signals this elevator's thread to stop after its current tick
    public void shutdown() {
        running = false;
    }

    @Override
    public void run() {
        running = true;
        System.out.println(Thread.currentThread().getName() + " started for Elevator " + id);
        while (running) {
            step();
            try {
                Thread.sleep(tickDurationMillis);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                running = false;
            }
        }
        System.out.println(Thread.currentThread().getName() + " stopped for Elevator " + id);
    }

    // Whole step is synchronized: it's the atomic unit of "this elevator moved one tick"
    private synchronized void step() {
        if (doorState == DoorState.OPEN) {
            closeDoor();
            return;
        }

        if (direction == Direction.UP) {
            handleUpStep();
        } else if (direction == Direction.DOWN) {
            handleDownStep();
        }
        // if IDLE, nothing pending -> elevator simply waits for the next tick
    }

    private void handleUpStep() {
        if (!upStops.isEmpty()) {
            int target = upStops.first();
            moveTowards(target);
            if (currentFloor == target) {
                upStops.remove(target);
                openDoor();
            }
        } else if (!downStops.isEmpty()) {
            direction = Direction.DOWN;
        } else {
            direction = Direction.IDLE;
            state = ElevatorState.IDLE;
        }
    }

    private void handleDownStep() {
        if (!downStops.isEmpty()) {
            int target = downStops.first();
            moveTowards(target);
            if (currentFloor == target) {
                downStops.remove(target);
                openDoor();
            }
        } else if (!upStops.isEmpty()) {
            direction = Direction.UP;
        } else {
            direction = Direction.IDLE;
            state = ElevatorState.IDLE;
        }
    }

    private void moveTowards(int target) {
        state = ElevatorState.MOVING;
        if (currentFloor < target) {
            currentFloor++;
        } else if (currentFloor > target) {
            currentFloor--;
        }
    }

    private void openDoor() {
        doorState = DoorState.OPEN;
        state = ElevatorState.STOPPED;
        System.out.println("Elevator " + id + " -> door OPEN at floor " + currentFloor);
    }

    private void closeDoor() {
        doorState = DoorState.CLOSED;
        System.out.println("Elevator " + id + " -> door CLOSED at floor " + currentFloor);
    }
}

// ---------- Scheduling Strategy (Strategy Pattern) ----------

interface SchedulingStrategy {
    Elevator selectElevator(List<Elevator> elevators, Request request);
}

class NearestElevatorStrategy implements SchedulingStrategy {

    @Override
    public Elevator selectElevator(List<Elevator> elevators, Request request) {
        Elevator best = null;
        int bestDistance = Integer.MAX_VALUE;

        for (Elevator elevator : elevators) {
            boolean suitable = elevator.getDirection() == Direction.IDLE
                    || elevator.getDirection() == request.getDirection();
            int distance = elevator.distanceTo(request.getFloor());
            if (suitable && distance < bestDistance) {
                bestDistance = distance;
                best = elevator;
            }
        }

        if (best == null) {
            for (Elevator elevator : elevators) {
                int distance = elevator.distanceTo(request.getFloor());
                if (distance < bestDistance) {
                    bestDistance = distance;
                    best = elevator;
                }
            }
        }
        return best;
    }
}

// ---------- Elevator Controller: owns thread lifecycle for all elevators ----------

class ElevatorController {

    private final List<Elevator> elevators;
    private final List<Thread> elevatorThreads;
    private final SchedulingStrategy strategy;

    public ElevatorController(List<Elevator> elevators, SchedulingStrategy strategy) {
        this.elevators = elevators;
        this.strategy = strategy;
        this.elevatorThreads = new ArrayList<>();
    }

    // Spins up one dedicated thread per elevator
    public void start() {
        for (Elevator elevator : elevators) {
            Thread thread = new Thread(elevator, "Elevator-" + elevator.getId() + "-Thread");
            elevatorThreads.add(thread);
            thread.start();
        }
    }

    // Synchronized: dispatch decisions must be serialized so two concurrent hall
    // calls don't both pick the same elevator off a stale snapshot of its state.
    public synchronized void requestElevator(int floor, Direction direction) {
        Request request = new Request(floor, direction);
        Elevator chosen = strategy.selectElevator(elevators, request);
        if (chosen != null) {
            System.out.println(Thread.currentThread().getName()
                    + " dispatching Elevator " + chosen.getId() + " to floor " + floor);
            chosen.addStop(floor);
        } else {
            System.out.println("No elevator available for floor " + floor);
        }
    }

    // Cabin call doesn't need controller-level locking: it targets one specific
    // elevator directly, and that elevator's own addStop() is already synchronized.
    public void selectFloor(int elevatorId, int destinationFloor) {
        for (Elevator elevator : elevators) {
            if (elevator.getId() == elevatorId) {
                elevator.addStop(destinationFloor);
                return;
            }
        }
    }

    // Signals every elevator to stop, then waits for all their threads to finish
    public void shutdown() throws InterruptedException {
        for (Elevator elevator : elevators) {
            elevator.shutdown();
        }
        for (Thread thread : elevatorThreads) {
            thread.join();
        }
    }

    public List<Elevator> getElevators() {
        return elevators;
    }
}

// ---------- Building (top-level aggregate) ----------

class Building {

    private final int numFloors;
    private final ElevatorController controller;

    public Building(int numFloors, ElevatorController controller) {
        this.numFloors = numFloors;
        this.controller = controller;
    }

    public ElevatorController getController() {
        return controller;
    }

    public int getNumFloors() {
        return numFloors;
    }
}

// ---------- Demo / Simulation ----------

public class ElevatorSystemDemo {

    // 'static' here is unavoidable: the JVM invokes main() without an instance.
    // Every other method/field in this file is instance-based, by design.
    public static void main(String[] args) throws InterruptedException {
        List<Elevator> elevators = new ArrayList<>();
        elevators.add(new Elevator(1, 8, 500));
        elevators.add(new Elevator(2, 8, 500));

        SchedulingStrategy strategy = new NearestElevatorStrategy();
        ElevatorController controller = new ElevatorController(elevators, strategy);
        Building building = new Building(10, controller);

        System.out.println("Starting building with " + elevators.size()
                + " elevators, each on its own thread.\n");
        controller.start();

        // Simulate multiple users concurrently pressing hall/cabin buttons from
        // different threads, hitting the controller at roughly the same time.
        ExecutorService userSimulator = Executors.newFixedThreadPool(4);
        userSimulator.submit(() -> controller.requestElevator(5, Direction.UP));
        userSimulator.submit(() -> controller.requestElevator(3, Direction.DOWN));
        userSimulator.submit(() -> controller.selectFloor(1, 8));
        userSimulator.submit(() -> controller.selectFloor(2, 0));

        userSimulator.shutdown();
        userSimulator.awaitTermination(5, TimeUnit.SECONDS);

        // Let elevators actually run for a while (real time, since each has its own thread)
        Thread.sleep(6000);

        controller.shutdown();

        System.out.println("\nFinal elevator positions:");
        for (Elevator elevator : controller.getElevators()) {
            System.out.println("Elevator " + elevator.getId() + " at floor "
                    + elevator.getCurrentFloor() + ", state=" + elevator.getState());
        }
    }
}
