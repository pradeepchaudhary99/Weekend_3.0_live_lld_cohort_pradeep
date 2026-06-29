from abc import ABC, abstractmethod
from typing import List


class UIComponent(ABC):
    @abstractmethod
    def render(self): pass


# Leaf components
class Logo(UIComponent):
    def render(self): print("   [LOGO]")

class SearchBar(UIComponent):
    def render(self): print("   Search Bar")

class ProfileIcon(UIComponent):
    def render(self): print("   Profile Icon")

class MenuItem(UIComponent):
    def __init__(self, name: str):
        self._name = name

    def render(self): print(f"   - {self._name}")

class WelcomeCard(UIComponent):
    def render(self): print("   Welcome Card")

class RevenueChart(UIComponent):
    def render(self): print("   Revenue Chart")

class OrdersTable(UIComponent):
    def render(self): print("   Orders Table")

class Footer(UIComponent):
    def render(self): print("   Footer")


# Composite components
class Header(UIComponent):
    def __init__(self):
        self._logo = Logo()
        self._search_bar = SearchBar()
        self._profile_icon = ProfileIcon()

    def render(self):
        print("Header")
        self._logo.render()
        self._search_bar.render()
        self._profile_icon.render()


class Sidebar(UIComponent):
    def __init__(self):
        self._menu_items: List[MenuItem] = [
            MenuItem("Home"),
            MenuItem("Analytics"),
            MenuItem("Orders"),
            MenuItem("Settings"),
        ]

    def render(self):
        print("Sidebar")
        for item in self._menu_items:
            item.render()


class Content(UIComponent):
    def __init__(self):
        self._welcome_card = WelcomeCard()
        self._revenue_chart = RevenueChart()
        self._orders_table = OrdersTable()

    def render(self):
        print("Content")
        self._welcome_card.render()
        self._revenue_chart.render()
        self._orders_table.render()


class Dashboard(UIComponent):
    def __init__(self):
        self._header = Header()
        self._sidebar = Sidebar()
        self._content = Content()
        self._footer = Footer()

    def render(self):
        print("\n====== DASHBOARD ======\n")
        self._header.render()
        print()
        self._sidebar.render()
        print()
        self._content.render()
        print()
        self._footer.render()
        print("\n=======================\n")


class DashboardV2(Dashboard):
    pass


if __name__ == "__main__":
    dashboard = Dashboard()
    dashboard.render()
