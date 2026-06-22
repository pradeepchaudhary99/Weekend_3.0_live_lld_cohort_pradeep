#include <iostream>
#include <memory>
using namespace std;

// Product interfaces
struct IButton  { virtual void renderButton() = 0; virtual ~IButton() = default; };
struct IModal   { virtual void renderModal()  = 0; virtual ~IModal()  = default; };
struct IScreen  { virtual void renderScreen() = 0; virtual ~IScreen() = default; };

// Windows products
struct WindowButton : IButton { void renderButton() override { cout << "Window button rendered\n"; } };
struct WindowModal  : IModal  { void renderModal()  override { cout << "Window modal rendered\n";  } };
struct WindowScreen : IScreen { void renderScreen() override { cout << "Window screen rendered\n"; } };

// Mac products
struct MacButton : IButton { void renderButton() override { cout << "Mac button rendered\n"; } };
struct MacModal  : IModal  { void renderModal()  override { cout << "Mac modal rendered\n";  } };
struct MacScreen : IScreen { void renderScreen() override { cout << "Mac screen rendered\n"; } };

// Linux products
struct LinuxButton : IButton { void renderButton() override { cout << "Mac button rendered\n"; } };
struct LinuxModal  : IModal  { void renderModal()  override { cout << "Mac modal rendered\n";  } };
struct LinuxScreen : IScreen { void renderScreen() override { cout << "Mac screen rendered\n"; } };

// Abstract factory
struct IUIFactory {
    virtual unique_ptr<IButton> getButton() = 0;
    virtual unique_ptr<IModal>  getModal()  = 0;
    virtual unique_ptr<IScreen> getScreen() = 0;
    virtual ~IUIFactory() = default;
};

struct LinuxFactory : IUIFactory {
    unique_ptr<IButton> getButton() override { return make_unique<LinuxButton>(); }
    unique_ptr<IModal>  getModal()  override { throw runtime_error("Unimplemented method 'getModal'"); }
    unique_ptr<IScreen> getScreen() override { throw runtime_error("Unimplemented method 'getScreen'"); }
};

struct WindowUIFactory : IUIFactory {
    unique_ptr<IButton> getButton() override { return make_unique<WindowButton>(); }
    unique_ptr<IModal>  getModal()  override { return make_unique<WindowModal>(); }
    unique_ptr<IScreen> getScreen() override { return make_unique<WindowScreen>(); }
};

struct MacUIFactory : IUIFactory {
    unique_ptr<IButton> getButton() override { return make_unique<MacButton>(); }
    unique_ptr<IModal>  getModal()  override { return make_unique<MacModal>(); }
    unique_ptr<IScreen> getScreen() override { return make_unique<MacScreen>(); }
};

class UIRender {
    unique_ptr<IButton> button;
    unique_ptr<IModal>  modal;
    unique_ptr<IScreen> screen;
public:
    UIRender(IUIFactory& factory) { toggleUI(factory); }

    void renderUI() {
        button->renderButton();
        modal->renderModal();
        screen->renderScreen();
    }

    void toggleUI(IUIFactory& factory) {
        button = factory.getButton();
        modal  = factory.getModal();
        screen = factory.getScreen();
        renderUI();
    }
};

int main() {
    WindowUIFactory windowFactory;
    MacUIFactory    macFactory;
    UIRender myView(windowFactory);
    myView.toggleUI(macFactory);
    return 0;
}
