#include <iostream>
#include <vector>
#include <memory>
#include <string>
using namespace std;

struct UIComponent {
    virtual void render() = 0;
    virtual ~UIComponent() = default;
};

// Leaf components
struct Logo        : UIComponent { void render() override { cout << "   [LOGO]\n"; } };
struct SearchBar   : UIComponent { void render() override { cout << "   Search Bar\n"; } };
struct ProfileIcon : UIComponent { void render() override { cout << "   Profile Icon\n"; } };
struct WelcomeCard : UIComponent { void render() override { cout << "   Welcome Card\n"; } };
struct RevenueChart: UIComponent { void render() override { cout << "   Revenue Chart\n"; } };
struct OrdersTable : UIComponent { void render() override { cout << "   Orders Table\n"; } };
struct Footer      : UIComponent { void render() override { cout << "   Footer\n"; } };

struct MenuItem : UIComponent {
    explicit MenuItem(string name) : name(move(name)) {}
    void render() override { cout << "   - " << name << "\n"; }
private:
    string name;
};

// Composite components
struct Header : UIComponent {
    void render() override {
        cout << "Header\n";
        Logo{}.render();
        SearchBar{}.render();
        ProfileIcon{}.render();
    }
};

struct Sidebar : UIComponent {
    Sidebar() {
        for (auto& n : {"Home", "Analytics", "Orders", "Settings"})
            items.push_back(make_unique<MenuItem>(n));
    }
    void render() override {
        cout << "Sidebar\n";
        for (auto& item : items) item->render();
    }
private:
    vector<unique_ptr<MenuItem>> items;
};

struct Content : UIComponent {
    void render() override {
        cout << "Content\n";
        WelcomeCard{}.render();
        RevenueChart{}.render();
        OrdersTable{}.render();
    }
};

struct Dashboard : UIComponent {
    void render() override {
        cout << "\n====== DASHBOARD ======\n\n";
        Header{}.render();   cout << "\n";
        Sidebar{}.render();  cout << "\n";
        Content{}.render();  cout << "\n";
        Footer{}.render();
        cout << "\n=======================\n\n";
    }
};

struct DashboardV2 : Dashboard {};

int main() {
    Dashboard dashboard;
    dashboard.render();
    return 0;
}
