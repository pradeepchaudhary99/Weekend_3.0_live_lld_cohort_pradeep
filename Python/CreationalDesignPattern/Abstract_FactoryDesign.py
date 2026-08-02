from abc import ABC, abstractmethod


# Products in UI

class IButton(ABC):
    @abstractmethod
    def render_button(self):
        pass


class IModal(ABC):
    @abstractmethod
    def render_modal(self):
        pass


class IScreen(ABC):
    @abstractmethod
    def render_screen(self):
        pass


# Windows
class WindowButton(IButton):
    def render_button(self):
        print("Window button rendered")


class WindowModal(IModal):
    def render_modal(self):
        print("Window modal rendered")


class WindowScreen(IScreen):
    def render_screen(self):
        print("Window screen rendered")


# Mac
class MacButton(IButton):
    def render_button(self):
        print("Mac button rendered")


class MacModal(IModal):
    def render_modal(self):
        print("Mac modal rendered")


class MacScreen(IScreen):
    def render_screen(self):
        print("Mac screen rendered")


# Linux
class LinuxButton(IButton):
    def render_button(self):
        print("Mac button rendered")


class LinuxModal(IModal):
    def render_modal(self):
        print("Mac modal rendered")


class LinuxScreen(IScreen):
    def render_screen(self):
        print("Mac screen rendered")


class IUIFactory(ABC):
    @abstractmethod
    def get_button(self) -> IButton:
        pass

    @abstractmethod
    def get_modal(self) -> IModal:
        pass

    @abstractmethod
    def get_screen(self) -> IScreen:
        pass


class LinuxFactory(IUIFactory):
    def get_button(self) -> IButton:
        return LinuxButton()

    def get_modal(self) -> IModal:
        raise NotImplementedError("Unimplemented method 'get_modal'")

    def get_screen(self) -> IScreen:
        raise NotImplementedError("Unimplemented method 'get_screen'")


class WindowUIFactory(IUIFactory):
    def get_button(self) -> IButton:
        return WindowButton()

    def get_modal(self) -> IModal:
        return WindowModal()

    def get_screen(self) -> IScreen:
        return WindowScreen()


class MacUIFactory(IUIFactory):
    def get_button(self) -> IButton:
        return MacButton()

    def get_modal(self) -> IModal:
        return MacModal()

    def get_screen(self) -> IScreen:
        return MacScreen()


class UIRender:
    def __init__(self, factory: IUIFactory):
        self.button = factory.get_button()
        self.modal = factory.get_modal()
        self.screen = factory.get_screen()
        self.render_ui()

    def render_ui(self):
        self.button.render_button()
        self.modal.render_modal()
        self.screen.render_screen()

    def toggle_ui(self, factory: IUIFactory):
        self.button = factory.get_button()
        self.modal = factory.get_modal()
        self.screen = factory.get_screen()
        self.render_ui()


def main():
    my_view = UIRender(WindowUIFactory())
    my_view.toggle_ui(MacUIFactory())


if __name__ == "__main__":
    main()
