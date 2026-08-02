class TV:
    def on(self):
        print("TV is ON")

    def set_input(self):
        print("TV input set to HDMI")


class SoundSystem:
    def on(self):
        print("Sound System is ON")

    def set_volume(self, volume: int):
        print(f"Volume set to {volume}")


class StreamingDevice:
    def on(self):
        print("Streaming Device is ON")

    def play_movie(self, movie: str):
        print(f"Playing movie: {movie}")


class HomeTheaterFacade:
    def __init__(self):
        self.tv = TV()
        self.sound_system = SoundSystem()
        self.streaming_device = StreamingDevice()

    def watch_movie(self, movie: str):
        print("Preparing Home Theater...\n")

        self.tv.on()
        self.tv.set_input()

        self.sound_system.on()
        self.sound_system.set_volume(20)

        self.streaming_device.on()
        self.streaming_device.play_movie(movie)

        print("\nEnjoy your movie!")


def main():
    home_theater = HomeTheaterFacade()
    home_theater.watch_movie("Interstellar")


if __name__ == "__main__":
    main()
