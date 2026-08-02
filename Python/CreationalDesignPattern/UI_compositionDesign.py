from abc import ABC, abstractmethod
from typing import List


# Every UI component follows this interface
class UIComponent(ABC):
    @abstractmethod
    def render(self):
        pass


# ----------------------
# Leaf Components
# ----------------------

class Logo(UIComponent):
    def render(self):
        print("   [LOGO]")


class SearchBar(UIComponent):
    def render(self):
        print("   Search Bar")


class ProfileIcon(UIComponent):
    def render(self):
        print("   Profile Icon")


class MenuItem(UIComponent):
    def __init__(self, name: str):
        self.name = name

    def render(self):
        print(f"   - {self.name}")


class WelcomeCard(UIComponent):
    def render(self):
        print("   Welcome Card")


class RevenueChart(UIComponent):
    def render(self):
        print("   Revenue Chart")


class OrdersTable(UIComponent):
    def render(self):
        print("   Orders Table")


class Footer(UIComponent):
    def render(self):
        print("   Footer")


# ----------------------
# Composite Components
# ----------------------

class Header(UIComponent):
    def __init__(self):
        self.logo = Logo()
        self.search_bar = SearchBar()
        self.profile_icon = ProfileIcon()

    def render(self):
        print("Header")
        self.logo.render()
        self.search_bar.render()
        self.profile_icon.render()


class Sidebar(UIComponent):
    def __init__(self):
        self.menu_items: List[MenuItem] = []
        self.menu_items.append(MenuItem("Home"))
        self.menu_items.append(MenuItem("Analytics"))
        self.menu_items.append(MenuItem("Orders"))
        self.menu_items.append(MenuItem("Settings"))

    def render(self):
        print("Sidebar")
        for item in self.menu_items:
            item.render()


class Content(UIComponent):
    def __init__(self):
        self.welcome_card = WelcomeCard()
        self.revenue_chart = RevenueChart()
        self.orders_table = OrdersTable()

    def render(self):
        print("Content")
        self.welcome_card.render()
        self.revenue_chart.render()
        self.orders_table.render()


# ----------------------
# Main Screen
# ----------------------

class Dashboard(UIComponent):
    def __init__(self):
        self.header = Header()
        self.sidebar = Sidebar()
        self.content = Content()
        self.footer = Footer()

    def render(self):
        print("\n====== DASHBOARD ======\n")

        self.header.render()
        print()

        self.sidebar.render()
        print()

        self.content.render()
        print()

        self.footer.render()

        print("\n=======================\n")


class DashboardV2(Dashboard):
    pass


# ----------------------
# Driver
# ----------------------

def main():
    dashboard = Dashboard()
    dashboard.render()


if __name__ == "__main__":
    main()
