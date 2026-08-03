/*
Zomato Food Delivery Demo
------------------------------

Functional Requirements:
    1. Search/Browse/Recommend by dish or by proximity to the customer's location
    2. Select the restaurant and explore it
    3. Add items to a cart
    4. Place order / make the payment / compute total
    5. Delivery partner assigned
    6. Track the order status

    ------ OUT OF SCOPE -----
    Support multiple types of payments
    Admin Service to manage the CRM

Non-Functional Requirements:
    No double assignment
    No lost/duplicate cart updates
    Valid state machine
    Extensibility

Entities:
    Customer
    FoodDeliveryService
    DeliveryPartner
    DeliveryPartnerAssignmentService
    RestrauntManagerService
    Restraunt, MenuItem
    Cart
    Order, OrderService
    PaymentGateway
*/

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

static int nextId = 1;

std::string generateId(const std::string& prefix) {
    return prefix + std::to_string(nextId++);
}

struct Location {
    double x;
    double y;

    double distanceTo(const Location& other) const {
        return std::sqrt(std::pow(x - other.x, 2) + std::pow(y - other.y, 2));
    }
};

struct MenuItem {
    std::string id;
    std::string name;
    double price;
    bool available;

    MenuItem(std::string name, double price)
        : id(generateId("item-")), name(std::move(name)), price(price), available(true) {}
};

class Restraunt {
public:
    std::string id;
    std::string name;
    Location location;
    bool open = true;

    Restraunt(std::string name, Location location)
        : id(generateId("rest-")), name(std::move(name)), location(location) {}

    void addMenuItem(std::shared_ptr<MenuItem> item) { menu[item->id] = item; }

    std::vector<std::shared_ptr<MenuItem>> getMenu() const {
        std::vector<std::shared_ptr<MenuItem>> items;
        for (auto& [id, item] : menu) items.push_back(item);
        return items;
    }

    bool hasDish(const std::string& dishName) const {
        for (auto& [id, item] : menu) {
            if (item->available && item->name == dishName) return true;
        }
        return false;
    }

private:
    std::map<std::string, std::shared_ptr<MenuItem>> menu;
};

class RestrauntManagerService {
public:
    std::shared_ptr<Restraunt> addRestraunt(std::shared_ptr<Restraunt> restraunt) {
        restraunts[restraunt->id] = restraunt;
        return restraunt;
    }

    std::vector<std::shared_ptr<Restraunt>> searchByDish(const std::string& dishName) const {
        std::vector<std::shared_ptr<Restraunt>> matches;
        for (auto& [id, r] : restraunts) {
            if (r->open && r->hasDish(dishName)) matches.push_back(r);
        }
        return matches;
    }

    std::vector<std::shared_ptr<Restraunt>> searchByProximity(const Location& customerLocation, double radius) const {
        std::vector<std::shared_ptr<Restraunt>> matches;
        for (auto& [id, r] : restraunts) {
            if (r->open && r->location.distanceTo(customerLocation) <= radius) matches.push_back(r);
        }
        return matches;
    }

private:
    std::map<std::string, std::shared_ptr<Restraunt>> restraunts;
};

class Cart {
public:
    mutable std::recursive_mutex lock;
    std::string restrauntId;
    std::map<std::shared_ptr<MenuItem>, int> items;

    void addItem(const std::shared_ptr<Restraunt>& restraunt, const std::shared_ptr<MenuItem>& item, int quantity) {
        std::lock_guard<std::recursive_mutex> guard(lock);
        if (!restrauntId.empty() && restrauntId != restraunt->id) {
            throw std::runtime_error("Cart already has items from another restraunt; clear it first");
        }
        restrauntId = restraunt->id;
        items[item] += quantity;
    }

    double total() const {
        std::lock_guard<std::recursive_mutex> guard(lock);
        double sum = 0;
        for (auto& [item, qty] : items) sum += item->price * qty;
        return sum;
    }

    void clear() {
        std::lock_guard<std::recursive_mutex> guard(lock);
        items.clear();
        restrauntId.clear();
    }
};

class Customer {
public:
    std::string id;
    std::string name;
    Location address;
    Cart cart;

    Customer(std::string name, Location address)
        : id(generateId("cust-")), name(std::move(name)), address(address) {}
};

class DeliveryPartner {
public:
    std::string id;
    std::string name;
    Location currentLocation;
    bool available = true;

    DeliveryPartner(std::string name, Location currentLocation)
        : id(generateId("partner-")), name(std::move(name)), currentLocation(currentLocation) {}
};

struct PaymentGateway {
    virtual bool pay(double amount) = 0;
    virtual ~PaymentGateway() = default;
};

class MockPaymentGateway : public PaymentGateway {
public:
    bool pay(double amount) override {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Charged " << amount << " via mock payment gateway" << std::endl;
        return true;
    }
};

enum class OrderStatus { PLACED, PARTNER_ASSIGNED, PICKED_UP, DELIVERED, CANCELLED };

std::string orderStatusName(OrderStatus status) {
    switch (status) {
        case OrderStatus::PLACED: return "PLACED";
        case OrderStatus::PARTNER_ASSIGNED: return "PARTNER_ASSIGNED";
        case OrderStatus::PICKED_UP: return "PICKED_UP";
        case OrderStatus::DELIVERED: return "DELIVERED";
        case OrderStatus::CANCELLED: return "CANCELLED";
    }
    return "UNKNOWN";
}

class Order {
public:
    std::string id;
    std::shared_ptr<Customer> customer;
    std::shared_ptr<Restraunt> restraunt;
    std::map<std::shared_ptr<MenuItem>, int> items;
    double totalAmount;
    OrderStatus status = OrderStatus::PLACED;
    std::shared_ptr<DeliveryPartner> deliveryPartner;

    Order(std::shared_ptr<Customer> customer, std::shared_ptr<Restraunt> restraunt,
          std::map<std::shared_ptr<MenuItem>, int> items, double totalAmount)
        : id(generateId("order-")), customer(std::move(customer)), restraunt(std::move(restraunt)),
          items(std::move(items)), totalAmount(totalAmount) {}
};

// Assigns the nearest available delivery partner to the restraunt; no partner
// is ever handed two active orders at once, which rules out double assignment.
class DeliveryPartnerAssignmentService {
public:
    void registerPartner(std::shared_ptr<DeliveryPartner> partner) { partners.push_back(std::move(partner)); }

    std::shared_ptr<DeliveryPartner> assign(const std::shared_ptr<Order>& order) {
        std::lock_guard<std::mutex> guard(mutex);
        std::shared_ptr<DeliveryPartner> nearest;
        double bestDistance = std::numeric_limits<double>::max();
        for (auto& partner : partners) {
            if (!partner->available) continue;
            double distance = partner->currentLocation.distanceTo(order->restraunt->location);
            if (distance < bestDistance) {
                bestDistance = distance;
                nearest = partner;
            }
        }
        if (nearest) nearest->available = false;
        return nearest;
    }

    void release(const std::shared_ptr<DeliveryPartner>& partner) {
        std::lock_guard<std::mutex> guard(mutex);
        partner->available = true;
    }

private:
    std::vector<std::shared_ptr<DeliveryPartner>> partners;
    std::mutex mutex;
};

class OrderService {
public:
    OrderService(std::shared_ptr<PaymentGateway> paymentGateway,
                 std::shared_ptr<DeliveryPartnerAssignmentService> assignmentService)
        : paymentGateway(std::move(paymentGateway)), assignmentService(std::move(assignmentService)) {}

    std::shared_ptr<Order> placeOrder(std::shared_ptr<Customer> customer, std::shared_ptr<Restraunt> restraunt) {
        Cart& cart = customer->cart;
        std::shared_ptr<Order> order;
        {
            std::lock_guard<std::recursive_mutex> guard(cart.lock);
            if (cart.items.empty()) {
                throw std::runtime_error("Cannot place an order with an empty cart");
            }
            order = std::make_shared<Order>(customer, restraunt, cart.items, cart.total());
            orders[order->id] = order;
            cart.items.clear();
            cart.restrauntId.clear();
        }

        paymentGateway->pay(order->totalAmount);

        auto partner = assignmentService->assign(order);
        if (partner) {
            order->deliveryPartner = partner;
            order->status = OrderStatus::PARTNER_ASSIGNED;
        }
        return order;
    }

    void advanceStatus(const std::string& orderId, OrderStatus status) {
        auto it = orders.find(orderId);
        if (it == orders.end()) throw std::invalid_argument("Unknown order: " + orderId);
        it->second->status = status;
        if (status == OrderStatus::DELIVERED && it->second->deliveryPartner) {
            assignmentService->release(it->second->deliveryPartner);
        }
    }

    std::shared_ptr<Order> trackOrder(const std::string& orderId) {
        auto it = orders.find(orderId);
        return it == orders.end() ? nullptr : it->second;
    }

private:
    std::shared_ptr<PaymentGateway> paymentGateway;
    std::shared_ptr<DeliveryPartnerAssignmentService> assignmentService;
    std::map<std::string, std::shared_ptr<Order>> orders;
};

class FoodDeliveryService {
public:
    RestrauntManagerService restrauntManagerService;
    std::shared_ptr<DeliveryPartnerAssignmentService> assignmentService =
        std::make_shared<DeliveryPartnerAssignmentService>();
    OrderService orderService{std::make_shared<MockPaymentGateway>(), assignmentService};
};

int main() {
    FoodDeliveryService service;

    auto tasty = service.restrauntManagerService.addRestraunt(
        std::make_shared<Restraunt>("Tasty Bites", Location{0, 0}));
    auto paneerTikka = std::make_shared<MenuItem>("Paneer Tikka", 220.0);
    auto naan = std::make_shared<MenuItem>("Butter Naan", 40.0);
    tasty->addMenuItem(paneerTikka);
    tasty->addMenuItem(naan);

    auto spice = service.restrauntManagerService.addRestraunt(
        std::make_shared<Restraunt>("Spice Route", Location{5, 5}));
    spice->addMenuItem(std::make_shared<MenuItem>("Chicken Biryani", 260.0));

    service.assignmentService->registerPartner(std::make_shared<DeliveryPartner>("Ravi", Location{0.5, 0.5}));
    service.assignmentService->registerPartner(std::make_shared<DeliveryPartner>("Neha", Location{4.5, 4.5}));

    auto alice = std::make_shared<Customer>("Alice", Location{1, 1});

    std::cout << "Search by dish 'Paneer Tikka': " << std::endl;
    for (auto& r : service.restrauntManagerService.searchByDish("Paneer Tikka")) {
        std::cout << "  - " << r->name << std::endl;
    }

    std::cout << "Search by proximity (radius 3 from Alice):" << std::endl;
    for (auto& r : service.restrauntManagerService.searchByProximity(alice->address, 3)) {
        std::cout << "  - " << r->name << std::endl;
    }

    alice->cart.addItem(tasty, paneerTikka, 1);
    alice->cart.addItem(tasty, naan, 2);
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Cart total: " << alice->cart.total() << std::endl;

    auto order = service.orderService.placeOrder(alice, tasty);
    std::cout << "Order placed: " << order->id << " status=" << orderStatusName(order->status)
              << " partner=" << (order->deliveryPartner ? order->deliveryPartner->name : "none") << std::endl;

    service.orderService.advanceStatus(order->id, OrderStatus::PICKED_UP);
    std::cout << "Order status: " << orderStatusName(service.orderService.trackOrder(order->id)->status) << std::endl;

    service.orderService.advanceStatus(order->id, OrderStatus::DELIVERED);
    std::cout << "Order status: " << orderStatusName(service.orderService.trackOrder(order->id)->status) << std::endl;

    return 0;
}
