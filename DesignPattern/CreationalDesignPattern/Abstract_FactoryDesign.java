package DesignPattern.CreationalDesignPattern;


//Products in UI

interface IButton{
    void renderButton();
}

interface IModal{
    void renderModal();
}

interface IScreen{
    void renderScreen();
}

//Windows 
class WindowButton implements IButton{
    @Override
    public void renderButton() {
        System.out.println("Window button rendered");
    }
}

class WindowModal implements IModal{

    @Override
    public void renderModal() {
        System.out.println("Window modal rendered");
    }  
}

class WindowScreen implements IScreen{
    @Override
    public void renderScreen() {
        System.out.println("Window screen rendered");
    }    
}

//Mac 
class MacButton implements IButton{
    @Override
    public void renderButton() {
        System.out.println("Mac button rendered");
    }
}

class MacModal implements IModal{

    @Override
    public void renderModal() {
        System.out.println("Mac modal rendered");
    }  
}

class MacScreen implements IScreen{
    @Override
    public void renderScreen() {
        System.out.println("Mac screen rendered");
    }    
}

//Linux

//Mac 
class LinuxButton implements IButton{
    @Override
    public void renderButton() {
        System.out.println("Mac button rendered");
    }
}

class LinuxModal implements IModal{

    @Override
    public void renderModal() {
        System.out.println("Mac modal rendered");
    }  
}

class LinuxScreen implements IScreen{
    @Override
    public void renderScreen() {
        System.out.println("Mac screen rendered");
    }    
}



interface I_UI_Factory{
    IButton getButton();
    IModal getModal();
    IScreen getScreen();
}

class LinuxFactory implements I_UI_Factory{

    @Override
    public IButton getButton() {
        return new LinuxButton();
    }

    @Override
    public IModal getModal() {
        // TODO Auto-generated method stub
        throw new UnsupportedOperationException("Unimplemented method 'getModal'");
    }

    @Override
    public IScreen getScreen() {
        // TODO Auto-generated method stub
        throw new UnsupportedOperationException("Unimplemented method 'getScreen'");
    }
    
}

class WindowUIFactory implements I_UI_Factory{
    @Override
    public IButton getButton() {
        return new WindowButton();
    }

    @Override
    public IModal getModal() {
        return new WindowModal();
    }

    @Override
    public IScreen getScreen() {
        return new WindowScreen();
    }
}


class MacUIFactory implements I_UI_Factory{
    @Override
    public IButton getButton() {
        return new MacButton();
    }

    @Override
    public IModal getModal() {
        return new MacModal();
    }

    @Override
    public IScreen getScreen() {
        return new MacScreen();
    }
}



class UIRender{
    IButton button;
    IModal modal;
    IScreen screen;

    public UIRender(I_UI_Factory factory){
        this.button = factory.getButton();
        this.modal = factory.getModal();
        this.screen = factory.getScreen();
        renderUI();
    }

    void renderUI(){
        button.renderButton();
        modal.renderModal();
        screen.renderScreen();
    }

    void toggleUI(I_UI_Factory factory){
        this.button = factory.getButton();
        this.modal = factory.getModal();
        this.screen = factory.getScreen();
        renderUI();
    }
}




public class Abstract_FactoryDesign {
    public static void main(String[] args) {
        UIRender myView = new UIRender(new WindowUIFactory());
        myView.toggleUI(new MacUIFactory());

    }
}
