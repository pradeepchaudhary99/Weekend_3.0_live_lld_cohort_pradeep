#include <iostream>
#include <string>
#include <vector>

// Every UI component follows this interface
struct UIComponent {
    virtual void render() = 0;
    virtual ~UIComponent() = default;
};

// ----------------------
// Leaf Components
// ----------------------

struct Logo : UIComponent {
    void render() override { std::cout << "   [LOGO]" << std::endl; }
};

struct SearchBar : UIComponent {
    void render() override { std::cout << "   Search Bar" << std::endl; }
};

struct ProfileIcon : UIComponent {
    void render() override { std::cout << "   Profile Icon" << std::endl; }
};

class MenuItem : public UIComponent {
    std::string name;

public:
    explicit MenuItem(std::string name) : name(std::move(name)) {}

    void render() override { std::cout << "   - " << name << std::endl; }
};

struct WelcomeCard : UIComponent {
    void render() override { std::cout << "   Welcome Card" << std::endl; }
};

struct RevenueChart : UIComponent {
    void render() override { std::cout << "   Revenue Chart" << std::endl; }
};

struct OrdersTable : UIComponent {
    void render() override { std::cout << "   Orders Table" << std::endl; }
};

struct Footer : UIComponent {
    void render() override { std::cout << "   Footer" << std::endl; }
};

// ----------------------
// Composite Components
// ----------------------

class Header : public UIComponent {
    Logo logo;
    SearchBar searchBar;
    ProfileIcon profileIcon;

public:
    void render() override {
        std::cout << "Header" << std::endl;
        logo.render();
        searchBar.render();
        profileIcon.render();
    }
};

class Sidebar : public UIComponent {
    std::vector<MenuItem> menuItems;

public:
    Sidebar() {
        menuItems.emplace_back("Home");
        menuItems.emplace_back("Analytics");
        menuItems.emplace_back("Orders");
        menuItems.emplace_back("Settings");
    }

    void render() override {
        std::cout << "Sidebar" << std::endl;
        for (auto& item : menuItems) {
            item.render();
        }
    }
};

class Content : public UIComponent {
    WelcomeCard welcomeCard;
    RevenueChart revenueChart;
    OrdersTable ordersTable;

public:
    void render() override {
        std::cout << "Content" << std::endl;
        welcomeCard.render();
        revenueChart.render();
        ordersTable.render();
    }
};

// ----------------------
// Main Screen
// ----------------------

class Dashboard : public UIComponent {
    Header header;
    Sidebar sidebar;
    Content content;
    Footer footer;

public:
    void render() override {
        std::cout << "\n====== DASHBOARD ======\n" << std::endl;

        header.render();
        std::cout << std::endl;

        sidebar.render();
        std::cout << std::endl;

        content.render();
        std::cout << std::endl;

        footer.render();

        std::cout << "\n=======================\n" << std::endl;
    }
};

class DashboardV2 : public Dashboard {};

// ----------------------
// Driver
// ----------------------

int main() {
    Dashboard dashboard;
    dashboard.render();
    return 0;
}
