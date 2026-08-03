//
/*

Functional Requirements:
    1. Search/Browse/Recommends  by dish or by proximity to the customers location
    2. Select the Restraunt and explore the restraunt
    3. Add items to a cart;
    4. Place order / Make the payments / compute total /
    5. Delivery partner Assigned
    6. Track the order status

    ------ OUT of SCOPE -----
    Support multiple types of payments
    Admin Service to manage the CRM


Non-Functional Requirements:
    No-Double Assignment
    No lost/duplicate cart updates
    valid state machine
    extensibility



Core Entities:

Customer

FoodDeliveryService

DeliveryPartner
DeliveryManagerService

RestrauntManagerService
Restraunt
Menu
MenuItem

Cart

Order
OrderService

PaymentGateway


*/

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.locks.ReentrantLock;

class Location {
    final double x;
    final double y;

    Location(double x, double y) {
        this.x = x;
        this.y = y;
    }

    double distanceTo(Location other) {
        return Math.sqrt(Math.pow(x - other.x, 2) + Math.pow(y - other.y, 2));
    }
}

class MenuItem {
    final String id;
    final String name;
    final double price;
    boolean available;

    MenuItem(String name, double price) {
        this.id = UUID.randomUUID().toString();
        this.name = name;
        this.price = price;
        this.available = true;
    }
}

class Restraunt {
    final String id;
    final String name;
    final Location location;
    final Map<String, MenuItem> menu = new LinkedHashMap<>();
    boolean open = true;

    Restraunt(String name, Location location) {
        this.id = UUID.randomUUID().toString();
        this.name = name;
        this.location = location;
    }

    void addMenuItem(MenuItem item) {
        menu.put(item.id, item);
    }

    List<MenuItem> getMenu() {
        return new ArrayList<>(menu.values());
    }

    boolean hasDish(String dishName) {
        for (MenuItem item : menu.values()) {
            if (item.available && item.name.equalsIgnoreCase(dishName)) {
                return true;
            }
        }
        return false;
    }
}

class RestrauntManagerService {
    private final Map<String, Restraunt> restraunts = new LinkedHashMap<>();

    Restraunt addRestraunt(Restraunt restraunt) {
        restraunts.put(restraunt.id, restraunt);
        return restraunt;
    }

    List<Restraunt> searchByDish(String dishName) {
        List<Restraunt> matches = new ArrayList<>();
        for (Restraunt restraunt : restraunts.values()) {
            if (restraunt.open && restraunt.hasDish(dishName)) {
                matches.add(restraunt);
            }
        }
        return matches;
    }

    List<Restraunt> searchByProximity(Location customerLocation, double radius) {
        List<Restraunt> matches = new ArrayList<>();
        for (Restraunt restraunt : restraunts.values()) {
            if (restraunt.open && restraunt.location.distanceTo(customerLocation) <= radius) {
                matches.add(restraunt);
            }
        }
        return matches;
    }
}

class Cart {
    final ReentrantLock lock = new ReentrantLock();
    String restrauntId;
    final Map<MenuItem, Integer> items = new LinkedHashMap<>();

    void addItem(Restraunt restraunt, MenuItem item, int quantity) {
        lock.lock();
        try {
            if (restrauntId != null && !restrauntId.equals(restraunt.id)) {
                throw new IllegalStateException("Cart already has items from another restraunt; clear it first");
            }
            restrauntId = restraunt.id;
            items.merge(item, quantity, Integer::sum);
        } finally {
            lock.unlock();
        }
    }

    double total() {
        lock.lock();
        try {
            double sum = 0;
            for (Map.Entry<MenuItem, Integer> entry : items.entrySet()) {
                sum += entry.getKey().price * entry.getValue();
            }
            return sum;
        } finally {
            lock.unlock();
        }
    }

    void clear() {
        lock.lock();
        try {
            items.clear();
            restrauntId = null;
        } finally {
            lock.unlock();
        }
    }
}

class Customer {
    final String id;
    final String name;
    final Location address;
    final Cart cart = new Cart();

    Customer(String name, Location address) {
        this.id = UUID.randomUUID().toString();
        this.name = name;
        this.address = address;
    }
}

class DeliveryPartner {
    final String id;
    final String name;
    Location currentLocation;
    boolean available = true;

    DeliveryPartner(String name, Location currentLocation) {
        this.id = UUID.randomUUID().toString();
        this.name = name;
        this.currentLocation = currentLocation;
    }
}

interface PaymentGateway {
    boolean pay(double amount);
}

class MockPaymentGateway implements PaymentGateway {
    @Override
    public boolean pay(double amount) {
        System.out.printf("Charged %.2f via mock payment gateway%n", amount);
        return true;
    }
}

enum OrderStatus {
    PLACED, PARTNER_ASSIGNED, PICKED_UP, DELIVERED, CANCELLED
}

class Order {
    final String id;
    final Customer customer;
    final Restraunt restraunt;
    final Map<MenuItem, Integer> items;
    final double totalAmount;
    OrderStatus status = OrderStatus.PLACED;
    DeliveryPartner deliveryPartner;

    Order(Customer customer, Restraunt restraunt, Map<MenuItem, Integer> items, double totalAmount) {
        this.id = UUID.randomUUID().toString();
        this.customer = customer;
        this.restraunt = restraunt;
        this.items = items;
        this.totalAmount = totalAmount;
    }
}

// Assigns the nearest available delivery partner to the restraunt; no partner
// is ever handed two active orders at once, which rules out double assignment.
class DeliveryPartnerAssignmentService {
    private final List<DeliveryPartner> partners = new ArrayList<>();

    void register(DeliveryPartner partner) {
        partners.add(partner);
    }

    synchronized DeliveryPartner assign(Order order) {
        DeliveryPartner nearest = null;
        double bestDistance = Double.MAX_VALUE;
        for (DeliveryPartner partner : partners) {
            if (!partner.available) {
                continue;
            }
            double distance = partner.currentLocation.distanceTo(order.restraunt.location);
            if (distance < bestDistance) {
                bestDistance = distance;
                nearest = partner;
            }
        }
        if (nearest != null) {
            nearest.available = false;
        }
        return nearest;
    }

    synchronized void release(DeliveryPartner partner) {
        partner.available = true;
    }
}

class OrderService {
    private final PaymentGateway paymentGateway;
    private final DeliveryPartnerAssignmentService assignmentService;
    private final Map<String, Order> orders = new LinkedHashMap<>();

    OrderService(PaymentGateway paymentGateway, DeliveryPartnerAssignmentService assignmentService) {
        this.paymentGateway = paymentGateway;
        this.assignmentService = assignmentService;
    }

    Order placeOrder(Customer customer, Restraunt restraunt) {
        Cart cart = customer.cart;
        cart.lock.lock();
        Order order;
        try {
            if (cart.items.isEmpty()) {
                throw new IllegalStateException("Cannot place an order with an empty cart");
            }
            order = new Order(customer, restraunt, new LinkedHashMap<>(cart.items), cart.total());
            orders.put(order.id, order);
            cart.clear();
        } finally {
            cart.lock.unlock();
        }

        paymentGateway.pay(order.totalAmount);

        DeliveryPartner partner = assignmentService.assign(order);
        if (partner != null) {
            order.deliveryPartner = partner;
            order.status = OrderStatus.PARTNER_ASSIGNED;
        }
        return order;
    }

    void advanceStatus(String orderId, OrderStatus status) {
        Order order = orders.get(orderId);
        if (order == null) {
            throw new IllegalArgumentException("Unknown order: " + orderId);
        }
        order.status = status;
        if (status == OrderStatus.DELIVERED && order.deliveryPartner != null) {
            assignmentService.release(order.deliveryPartner);
        }
    }

    Order trackOrder(String orderId) {
        return orders.get(orderId);
    }
}

class FoodDeliveryService {
    final RestrauntManagerService restrauntManagerService = new RestrauntManagerService();
    final DeliveryPartnerAssignmentService assignmentService = new DeliveryPartnerAssignmentService();
    final OrderService orderService = new OrderService(new MockPaymentGateway(), assignmentService);
}

public class ZomatoFoodDeliveryDemo {
    public static void main(String[] args) {
        FoodDeliveryService service = new FoodDeliveryService();

        Restraunt tasty = service.restrauntManagerService.addRestraunt(new Restraunt("Tasty Bites", new Location(0, 0)));
        MenuItem paneerTikka = new MenuItem("Paneer Tikka", 220.0);
        MenuItem naan = new MenuItem("Butter Naan", 40.0);
        tasty.addMenuItem(paneerTikka);
        tasty.addMenuItem(naan);

        Restraunt spice = service.restrauntManagerService.addRestraunt(new Restraunt("Spice Route", new Location(5, 5)));
        spice.addMenuItem(new MenuItem("Chicken Biryani", 260.0));

        service.assignmentService.register(new DeliveryPartner("Ravi", new Location(0.5, 0.5)));
        service.assignmentService.register(new DeliveryPartner("Neha", new Location(4.5, 4.5)));

        Customer alice = new Customer("Alice", new Location(1, 1));

        System.out.println("Search by dish 'Paneer Tikka': ");
        for (Restraunt r : service.restrauntManagerService.searchByDish("Paneer Tikka")) {
            System.out.println("  - " + r.name);
        }

        System.out.println("Search by proximity (radius 3 from Alice):");
        for (Restraunt r : service.restrauntManagerService.searchByProximity(alice.address, 3)) {
            System.out.println("  - " + r.name);
        }

        alice.cart.addItem(tasty, paneerTikka, 1);
        alice.cart.addItem(tasty, naan, 2);
        System.out.printf("Cart total: %.2f%n", alice.cart.total());

        Order order = service.orderService.placeOrder(alice, tasty);
        System.out.println("Order placed: " + order.id + " status=" + order.status
                + " partner=" + (order.deliveryPartner != null ? order.deliveryPartner.name : "none"));

        service.orderService.advanceStatus(order.id, OrderStatus.PICKED_UP);
        System.out.println("Order status: " + service.orderService.trackOrder(order.id).status);

        service.orderService.advanceStatus(order.id, OrderStatus.DELIVERED);
        System.out.println("Order status: " + service.orderService.trackOrder(order.id).status);
    }
}
