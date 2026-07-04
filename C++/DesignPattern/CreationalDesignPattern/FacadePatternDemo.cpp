#include <iostream>
#include <memory>
using namespace std;

// Subsystem 1
struct TV {
    void on() {
        cout << "TV is ON\n";
    }

    void setInput() {
        cout << "TV input set to HDMI\n";
    }
};

// Subsystem 2
struct SoundSystem {
    void on() {
        cout << "Sound System is ON\n";
    }

    void setVolume(int volume) {
        cout << "Volume set to " << volume << "\n";
    }
};

// Subsystem 3
struct StreamingDevice {
    void on() {
        cout << "Streaming Device is ON\n";
    }

    void playMovie(const string& movie) {
        cout << "Playing movie: " << movie << "\n";
    }
};

// Facade
class HomeTheaterFacade {
    unique_ptr<TV> tv;
    unique_ptr<SoundSystem> soundSystem;
    unique_ptr<StreamingDevice> streamingDevice;

public:
    HomeTheaterFacade()
        : tv(make_unique<TV>()),
          soundSystem(make_unique<SoundSystem>()),
          streamingDevice(make_unique<StreamingDevice>()) {}

    void watchMovie(const string& movie) {
        cout << "Preparing Home Theater...\n\n";

        tv->on();
        tv->setInput();

        soundSystem->on();
        soundSystem->setVolume(20);

        streamingDevice->on();
        streamingDevice->playMovie(movie);

        cout << "\nEnjoy your movie!\n";
    }
};

int main() {
    HomeTheaterFacade homeTheater;
    homeTheater.watchMovie("Interstellar");
    return 0;
}
