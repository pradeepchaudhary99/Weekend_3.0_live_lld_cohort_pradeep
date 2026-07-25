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

import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicLong;

// ---------- Enums ----------

enum VehicleType {
    MOTORCYCLE, CAR, TRUCK
}

enum SpotType {
    SMALL, MEDIUM, LARGE
}

// ---------- Vehicle hierarchy ----------

abstract class Vehicle {
    private final String licencePlate;
    private final VehicleType type;

    protected Vehicle(String licencePlate, VehicleType type) {
        this.licencePlate = licencePlate;
        this.type = type;
    }

    public String getLicencePlate() {
        return licencePlate;
    }

    public VehicleType getType() {
        return type;
    }

    // A vehicle can use any spot at least as big as what it strictly needs.
    public abstract boolean canFitIn(SpotType spotType);
}

class Motorcycle extends Vehicle {
    public Motorcycle(String licencePlate) {
        super(licencePlate, VehicleType.MOTORCYCLE);
    }

    @Override
    public boolean canFitIn(SpotType spotType) {
        return true; // fits any spot
    }
}

class Car extends Vehicle {
    public Car(String licencePlate) {
        super(licencePlate, VehicleType.CAR);
    }

    @Override
    public boolean canFitIn(SpotType spotType) {
        return spotType == SpotType.MEDIUM || spotType == SpotType.LARGE;
    }
}

class Truck extends Vehicle {
    public Truck(String licencePlate) {
        super(licencePlate, VehicleType.TRUCK);
    }

    @Override
    public boolean canFitIn(SpotType spotType) {
        return spotType == SpotType.LARGE;
    }
}

// ---------- Parking spot ----------

// Occupancy is claimed with tryPark(), which is synchronized so two threads
// racing for the same spot can never both succeed.
class ParkingSpot {
    private final String spotId;
    private final int levelNumber;
    private final SpotType spotType;
    private Vehicle vehicle;

    public ParkingSpot(String spotId, int levelNumber, SpotType spotType) {
        this.spotId = spotId;
        this.levelNumber = levelNumber;
        this.spotType = spotType;
    }

    public String getSpotId() {
        return spotId;
    }

    public int getLevelNumber() {
        return levelNumber;
    }

    public SpotType getSpotType() {
        return spotType;
    }

    public synchronized boolean isOccupied() {
        return vehicle != null;
    }

    public synchronized boolean tryPark(Vehicle candidate) {
        if (vehicle != null) {
            return false;
        }
        vehicle = candidate;
        return true;
    }

    public synchronized Vehicle unpark() {
        Vehicle parked = vehicle;
        vehicle = null;
        return parked;
    }
}

// ---------- Parking level ----------

class ParkingLevel {
    private final int levelNumber;
    private final List<ParkingSpot> spots;

    public ParkingLevel(int levelNumber, List<ParkingSpot> spots) {
        this.levelNumber = levelNumber;
        this.spots = spots;
    }

    public int getLevelNumber() {
        return levelNumber;
    }

    public List<ParkingSpot> getSpots() {
        return spots;
    }

    public long availableCount(SpotType spotType) {
        return spots.stream()
                .filter(s -> s.getSpotType() == spotType && !s.isOccupied())
                .count();
    }
}

// ---------- Spot assignment strategy (Strategy pattern) ----------

interface SpotAssignmentStrategy {
    // Returns candidate spots in the order they should be attempted; the
    // caller races tryPark() over them so a losing candidate just moves on.
    List<ParkingSpot> candidateSpots(List<ParkingLevel> levels, Vehicle vehicle);
}

class FirstFitStrategy implements SpotAssignmentStrategy {
    @Override
    public List<ParkingSpot> candidateSpots(List<ParkingLevel> levels, Vehicle vehicle) {
        List<ParkingSpot> candidates = new ArrayList<>();
        for (ParkingLevel level : levels) {
            for (ParkingSpot spot : level.getSpots()) {
                if (vehicle.canFitIn(spot.getSpotType())) {
                    candidates.add(spot);
                }
            }
        }
        return candidates;
    }
}

// Prefers the smallest spot type the vehicle fits in, to save larger spots
// for vehicles that actually need them.
class BestFitStrategy implements SpotAssignmentStrategy {
    private static final List<SpotType> SIZE_ORDER = List.of(SpotType.SMALL, SpotType.MEDIUM, SpotType.LARGE);

    @Override
    public List<ParkingSpot> candidateSpots(List<ParkingLevel> levels, Vehicle vehicle) {
        List<ParkingSpot> candidates = new ArrayList<>();
        for (SpotType type : SIZE_ORDER) {
            if (!vehicle.canFitIn(type)) {
                continue;
            }
            for (ParkingLevel level : levels) {
                for (ParkingSpot spot : level.getSpots()) {
                    if (spot.getSpotType() == type) {
                        candidates.add(spot);
                    }
                }
            }
        }
        return candidates;
    }
}

// ---------- Ticket ----------

class Ticket {
    private final String ticketId;
    private final Vehicle vehicle;
    private final ParkingSpot spot;
    private final long entryTimeMillis;

    public Ticket(String ticketId, Vehicle vehicle, ParkingSpot spot, long entryTimeMillis) {
        this.ticketId = ticketId;
        this.vehicle = vehicle;
        this.spot = spot;
        this.entryTimeMillis = entryTimeMillis;
    }

    public String getTicketId() {
        return ticketId;
    }

    public Vehicle getVehicle() {
        return vehicle;
    }

    public ParkingSpot getSpot() {
        return spot;
    }

    public long getEntryTimeMillis() {
        return entryTimeMillis;
    }
}

class TicketGenerator {
    private final AtomicLong sequence = new AtomicLong();

    public String nextTicketId() {
        return "T-" + sequence.incrementAndGet();
    }
}

// ---------- Payment (Strategy pattern, decorated) ----------

interface PaymentStrategy {
    double calculateAndCharge(Ticket ticket, long exitTimeMillis);
}

class HourlyRatePayment implements PaymentStrategy {
    private static final double RATE_PER_HOUR = 20.0;

    @Override
    public double calculateAndCharge(Ticket ticket, long exitTimeMillis) {
        long durationMillis = Math.max(0, exitTimeMillis - ticket.getEntryTimeMillis());
        double hours = Math.max(1.0, durationMillis / 3_600_000.0); // minimum 1 hour billed
        double amount = hours * RATE_PER_HOUR;
        System.out.printf("Charging %.2f for ticket %s (%.2f hours)%n", amount, ticket.getTicketId(), hours);
        return amount;
    }
}

// Decorator pattern: wraps any PaymentStrategy to apply a membership discount
// without the base strategy knowing anything about memberships.
abstract class PaymentDecorator implements PaymentStrategy {
    protected final PaymentStrategy delegate;

    protected PaymentDecorator(PaymentStrategy delegate) {
        this.delegate = delegate;
    }
}

class MembershipDiscountDecorator extends PaymentDecorator {
    private final double discountPercent;

    public MembershipDiscountDecorator(PaymentStrategy delegate, double discountPercent) {
        super(delegate);
        this.discountPercent = discountPercent;
    }

    @Override
    public double calculateAndCharge(Ticket ticket, long exitTimeMillis) {
        double base = delegate.calculateAndCharge(ticket, exitTimeMillis);
        double discounted = base * (1 - discountPercent / 100.0);
        System.out.printf("Applying %.0f%% membership discount -> %.2f%n", discountPercent, discounted);
        return discounted;
    }
}

// ---------- Observer pattern: display boards ----------

interface DisplayDevice {
    void update(String data);
}

class Screen implements DisplayDevice {
    private final String name;

    public Screen(String name) {
        this.name = name;
    }

    @Override
    public void update(String data) {
        System.out.println("[" + name + "] " + data);
    }
}

// ---------- Gates ----------

class EntryGate {
    private final ParkingLotManager manager;
    private final TicketGenerator ticketGenerator;

    public EntryGate(ParkingLotManager manager, TicketGenerator ticketGenerator) {
        this.manager = manager;
        this.ticketGenerator = ticketGenerator;
    }

    public Optional<Ticket> issueTicket(Vehicle vehicle) {
        Optional<ParkingSpot> spot = manager.parkVehicle(vehicle);
        if (spot.isEmpty()) {
            System.out.println("No spot available for " + vehicle.getLicencePlate());
            return Optional.empty();
        }
        Ticket ticket = new Ticket(ticketGenerator.nextTicketId(), vehicle, spot.get(), System.currentTimeMillis());
        manager.registerTicket(ticket);
        System.out.println("Issued " + ticket.getTicketId() + " to " + vehicle.getLicencePlate()
                + " at spot " + spot.get().getSpotId());
        return Optional.of(ticket);
    }
}

class ExitGate {
    private final ParkingLotManager manager;
    private final PaymentStrategy paymentStrategy;

    public ExitGate(ParkingLotManager manager, PaymentStrategy paymentStrategy) {
        this.manager = manager;
        this.paymentStrategy = paymentStrategy;
    }

    public boolean processExit(String ticketId) {
        Optional<Ticket> ticket = manager.releaseTicket(ticketId);
        if (ticket.isEmpty()) {
            System.out.println("Unknown ticket " + ticketId);
            return false;
        }
        paymentStrategy.calculateAndCharge(ticket.get(), System.currentTimeMillis());
        return true;
    }
}

// ---------- Parking lot manager (orchestrator) ----------

class ParkingLotManager {
    private final List<ParkingLevel> levels;
    private final SpotAssignmentStrategy assignmentStrategy;
    private final List<DisplayDevice> displayDevices = new CopyOnWriteArrayList<>();
    private final ConcurrentHashMap<String, Ticket> activeTickets = new ConcurrentHashMap<>();

    public ParkingLotManager(List<ParkingLevel> levels, SpotAssignmentStrategy assignmentStrategy) {
        this.levels = levels;
        this.assignmentStrategy = assignmentStrategy;
    }

    public void addDisplayDevice(DisplayDevice device) {
        displayDevices.add(device);
    }

    // Races candidate spots one at a time: tryPark() is atomic per-spot, so
    // concurrent callers can never double-park the same spot, and no global
    // lock is needed across the whole manager.
    public Optional<ParkingSpot> parkVehicle(Vehicle vehicle) {
        List<ParkingSpot> candidates = assignmentStrategy.candidateSpots(levels, vehicle);
        for (ParkingSpot spot : candidates) {
            if (spot.tryPark(vehicle)) {
                updateDisplay();
                return Optional.of(spot);
            }
        }
        return Optional.empty();
    }

    public void registerTicket(Ticket ticket) {
        activeTickets.put(ticket.getTicketId(), ticket);
    }

    public Optional<Ticket> releaseTicket(String ticketId) {
        Ticket ticket = activeTickets.remove(ticketId);
        if (ticket == null) {
            return Optional.empty();
        }
        ticket.getSpot().unpark();
        updateDisplay();
        return Optional.of(ticket);
    }

    public long getAvailableSpotCount(SpotType spotType) {
        return levels.stream().mapToLong(level -> level.availableCount(spotType)).sum();
    }

    public void updateDisplay() {
        StringBuilder summary = new StringBuilder("Availability -> ");
        for (SpotType type : SpotType.values()) {
            summary.append(type).append(": ").append(getAvailableSpotCount(type)).append("  ");
        }
        String data = summary.toString();
        for (DisplayDevice device : displayDevices) {
            device.update(data);
        }
    }
}

// ---------- Demo / Simulation ----------

public class ParkingLotDemo {

    private static List<ParkingLevel> buildLevels(int numLevels, int spotsPerType) {
        List<ParkingLevel> levels = new ArrayList<>();
        for (int lvl = 1; lvl <= numLevels; lvl++) {
            List<ParkingSpot> spots = new ArrayList<>();
            for (SpotType type : SpotType.values()) {
                for (int i = 1; i <= spotsPerType; i++) {
                    spots.add(new ParkingSpot("L" + lvl + "-" + type + "-" + i, lvl, type));
                }
            }
            levels.add(new ParkingLevel(lvl, spots));
        }
        return levels;
    }

    public static void main(String[] args) throws InterruptedException {
        List<ParkingLevel> levels = buildLevels(2, 2); // 2 levels x 2 spots per type = 12 spots total
        ParkingLotManager manager = new ParkingLotManager(levels, new BestFitStrategy());
        manager.addDisplayDevice(new Screen("EntranceDisplay"));

        TicketGenerator ticketGenerator = new TicketGenerator();
        EntryGate entryGate = new EntryGate(manager, ticketGenerator);
        PaymentStrategy payment = new MembershipDiscountDecorator(new HourlyRatePayment(), 10);
        ExitGate exitGate = new ExitGate(manager, payment);

        List<Vehicle> incoming = List.of(
                new Motorcycle("MC-1"), new Car("CAR-1"), new Car("CAR-2"),
                new Truck("TRK-1"), new Motorcycle("MC-2"), new Car("CAR-3"));

        // Simulate multiple vehicles arriving concurrently from different threads.
        ExecutorService entryPool = Executors.newFixedThreadPool(4);
        CountDownLatch entryLatch = new CountDownLatch(incoming.size());
        List<Ticket> issuedTickets = new CopyOnWriteArrayList<>();

        for (Vehicle vehicle : incoming) {
            entryPool.submit(() -> {
                try {
                    entryGate.issueTicket(vehicle).ifPresent(issuedTickets::add);
                } finally {
                    entryLatch.countDown();
                }
            });
        }
        entryLatch.await();
        entryPool.shutdown();
        entryPool.awaitTermination(5, TimeUnit.SECONDS);

        manager.updateDisplay();

        // A couple of vehicles leave.
        if (!issuedTickets.isEmpty()) {
            exitGate.processExit(issuedTickets.get(0).getTicketId());
        }
        if (issuedTickets.size() > 1) {
            exitGate.processExit(issuedTickets.get(1).getTicketId());
        }

        manager.updateDisplay();
        System.out.println("Tickets currently active: " + (issuedTickets.size() - 2));
    }
}
