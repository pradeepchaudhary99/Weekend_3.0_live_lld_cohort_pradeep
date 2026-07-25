package CreationalDesignPattern;
import java.util.*;

// Every UI component follows this interface
interface UIComponent {
    void render();
}

// ----------------------
// Leaf Components
// ----------------------

class Logo implements UIComponent {
    public void render() {
        System.out.println("   [LOGO]");
    }
}

class SearchBar implements UIComponent {
    public void render() {
        System.out.println("   Search Bar");
    }
}

class ProfileIcon implements UIComponent {
    public void render() {
        System.out.println("   Profile Icon");
    }
}

class MenuItem implements UIComponent {
    private String name;

    public MenuItem(String name) {
        this.name = name;
    }

    public void render() {
        System.out.println("   - " + name);
    }
}

class WelcomeCard implements UIComponent {
    public void render() {
        System.out.println("   Welcome Card");
    }
}

class RevenueChart implements UIComponent {
    public void render() {
        System.out.println("   Revenue Chart");
    }
}

class OrdersTable implements UIComponent {
    public void render() {
        System.out.println("   Orders Table");
    }
}

class Footer implements UIComponent {
    public void render() {
        System.out.println("   Footer");
    }
}

// ----------------------
// Composite Components
// ----------------------

class Header implements UIComponent {

    private Logo logo = new Logo();
    private SearchBar searchBar = new SearchBar();
    private ProfileIcon profileIcon = new ProfileIcon();

    public void render() {
        System.out.println("Header");
        logo.render();
        searchBar.render();
        profileIcon.render();
    }
}



class Sidebar implements UIComponent {

    private List<MenuItem> menuItems = new ArrayList<>();

    public Sidebar() {
        menuItems.add(new MenuItem("Home"));
        menuItems.add(new MenuItem("Analytics"));
        menuItems.add(new MenuItem("Orders"));
        menuItems.add(new MenuItem("Settings"));
    }

    public void render() {
        System.out.println("Sidebar");
        for (MenuItem item : menuItems) {
            item.render();
        }
    }
}

class Content implements UIComponent {
    private WelcomeCard welcomeCard = new WelcomeCard();
    private RevenueChart revenueChart = new RevenueChart();
    private OrdersTable ordersTable = new OrdersTable();

    public void render() {
        System.out.println("Content");
        welcomeCard.render();
        revenueChart.render();
        ordersTable.render();
    }
}

// ----------------------
// Main Screen
// ----------------------

class Dashboard implements UIComponent {

    private Header header = new Header();
    private Sidebar sidebar = new Sidebar();
    private Content content = new Content();
    private Footer footer = new Footer();

    public void render() {
        System.out.println("\n====== DASHBOARD ======\n");

        header.render();
        System.out.println();

        sidebar.render();
        System.out.println();

        content.render();
        System.out.println();

        footer.render();

        System.out.println("\n=======================\n");
    }
}

class DashboardV2 extends Dashboard{

}


// ----------------------
// Driver
// ----------------------

public class UI_compositionDesign {

    public static void main(String[] args) {

        Dashboard dashboard = new Dashboard();

        dashboard.render();
    }
}