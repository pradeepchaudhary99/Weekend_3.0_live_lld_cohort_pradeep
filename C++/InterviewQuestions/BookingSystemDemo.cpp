/*
TicketMaster
BookMyShow
HotelBookingSystem


// 20% part will give 80% result
1. Idempotency
2. Avoid Double Booking
3. Seat Locking


FR:
    User should be able to search for available seats
    User should be able to book one or more seats
    User can cancel a booking
    Prevent double booking of the same seat
    Support multiple payment methods


NFR:
    Thread-safety: two users racing for overlapping seats must never both win
    Atomicity: locking a multi-seat request is all-or-nothing
    Deadlock-free: concurrent multi-seat locks never wait on each other in a cycle


Entities:

User
Seat
Screen/Audi
Cinema
Show
Movie
ShowSeat

Booking
BookingStatus
ShowSeatStatus (AVAILABLE, LOCKED, BOOKED)

BookingService
    --> BookingRepository
    --> PaymentGateway

    searchShow(pattern)
    bookSeats(user, show, seats)
*/

#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

enum class ShowSeatStatus { AVAILABLE, LOCKED, BOOKED };

enum class BookingStatus { PENDING, CONFIRMED, CANCELLED, FAILED };

enum class SeatType { PREMIUM, SILVER, PLATINUM };

class User {
public:
    explicit User(std::string userId) : userId_(std::move(userId)) {}
    const std::string& getUserId() const { return userId_; }

private:
    std::string userId_;
};

class Movie {
public:
    Movie(std::string movieId, std::string title) : movieId_(std::move(movieId)), title_(std::move(title)) {}

private:
    std::string movieId_;
    std::string title_;
};

class Seat {
public:
    Seat(std::string seatId, SeatType type) : seatId_(std::move(seatId)), type_(type) {}
    SeatType getType() const { return type_; }

private:
    std::string seatId_;
    SeatType type_;
};

// One seat for one specific show. Its own mutex guards status transitions;
// multi-seat operations lock these mutexes in a globally consistent order
// (sorted by showSeatId) so concurrent bookings can never deadlock on them.
class ShowSeat {
public:
    ShowSeat(std::string showSeatId, std::shared_ptr<Seat> seat)
        : showSeatId_(std::move(showSeatId)), seat_(std::move(seat)), status_(ShowSeatStatus::AVAILABLE) {}

    const std::string& getShowSeatId() const { return showSeatId_; }
    ShowSeatStatus getStatus() const { return status_; }
    void setStatus(ShowSeatStatus status) { status_ = status; }
    std::mutex& getMutex() { return mutex_; }

private:
    std::string showSeatId_;
    std::shared_ptr<Seat> seat_;
    ShowSeatStatus status_;
    std::mutex mutex_;
};

class Screen {
public:
    std::vector<std::shared_ptr<class Show>> allShows_;
};

class Show {
public:
    Show(std::shared_ptr<Movie> movie, std::chrono::system_clock::time_point start,
         std::chrono::system_clock::time_point endTime, std::shared_ptr<Screen> screen)
        : movie_(std::move(movie)), start_(start), endTime_(endTime), screen_(std::move(screen)) {}

    void addSeat(std::shared_ptr<ShowSeat> seat) {
        showSeats_[seat->getShowSeatId()] = seat;
    }

    const std::unordered_map<std::string, std::shared_ptr<ShowSeat>>& getShowSeats() const {
        return showSeats_;
    }

    // Atomically locks every seat in `seats`, or none of them.
    //
    // Deadlock avoidance: two concurrent calls that both touch seats {A, B}
    // (regardless of the caller's order) always acquire A before B, because
    // we sort by showSeatId first. That total lock order rules out the
    // classic "thread 1 holds A wants B, thread 2 holds B wants A" cycle.
    bool lockSeats(std::vector<std::shared_ptr<ShowSeat>> seats) {
        std::sort(seats.begin(), seats.end(), [](const auto& a, const auto& b) {
            return a->getShowSeatId() < b->getShowSeatId();
        });

        std::vector<std::unique_lock<std::mutex>> acquired;
        acquired.reserve(seats.size());
        for (auto& seat : seats) {
            acquired.emplace_back(seat->getMutex());
        }

        for (auto& seat : seats) {
            if (seat->getStatus() != ShowSeatStatus::AVAILABLE) {
                return false; // unique_locks unwind and release automatically
            }
        }
        for (auto& seat : seats) {
            seat->setStatus(ShowSeatStatus::LOCKED);
        }
        return true;
    }

    void unlockSeats(const std::vector<std::shared_ptr<ShowSeat>>& seats) {
        for (auto& seat : seats) {
            std::lock_guard<std::mutex> lock(seat->getMutex());
            if (seat->getStatus() == ShowSeatStatus::LOCKED) {
                seat->setStatus(ShowSeatStatus::AVAILABLE);
            }
        }
    }

    void markBooked(const std::vector<std::shared_ptr<ShowSeat>>& seats) {
        for (auto& seat : seats) {
            std::lock_guard<std::mutex> lock(seat->getMutex());
            seat->setStatus(ShowSeatStatus::BOOKED);
        }
    }

    // Cancellation: BOOKED seats go back to AVAILABLE for resale.
    void releaseBookedSeats(const std::vector<std::shared_ptr<ShowSeat>>& seats) {
        for (auto& seat : seats) {
            std::lock_guard<std::mutex> lock(seat->getMutex());
            if (seat->getStatus() == ShowSeatStatus::BOOKED) {
                seat->setStatus(ShowSeatStatus::AVAILABLE);
            }
        }
    }

    std::vector<std::shared_ptr<ShowSeat>> findAvailableSeats() const {
        std::vector<std::shared_ptr<ShowSeat>> available;
        for (auto& [id, seat] : showSeats_) {
            if (seat->getStatus() == ShowSeatStatus::AVAILABLE) {
                available.push_back(seat);
            }
        }
        return available;
    }

private:
    std::shared_ptr<Movie> movie_;
    std::chrono::system_clock::time_point start_;
    std::chrono::system_clock::time_point endTime_;
    std::shared_ptr<Screen> screen_;
    std::unordered_map<std::string, std::shared_ptr<ShowSeat>> showSeats_;
};

class Cinema {
public:
    Cinema(std::string cinemaId, std::string address) : cinemaId_(std::move(cinemaId)), address_(std::move(address)) {}

private:
    std::string cinemaId_;
    std::string address_;
    std::vector<std::shared_ptr<Screen>> screens_;
};

class Booking {
public:
    Booking(std::string bookingId, std::shared_ptr<User> user, std::shared_ptr<Show> show,
            std::vector<std::shared_ptr<ShowSeat>> seats)
        : bookingId_(std::move(bookingId)), user_(std::move(user)), show_(std::move(show)),
          seats_(std::move(seats)), status_(BookingStatus::PENDING) {}

    const std::string& getBookingId() const { return bookingId_; }
    const std::shared_ptr<Show>& getShow() const { return show_; }
    const std::vector<std::shared_ptr<ShowSeat>>& getSeats() const { return seats_; }
    BookingStatus getStatus() const { return status_; }
    void setStatus(BookingStatus status) { status_ = status; }

private:
    std::string bookingId_;
    std::shared_ptr<User> user_;
    std::shared_ptr<Show> show_;
    std::vector<std::shared_ptr<ShowSeat>> seats_;
    BookingStatus status_;
};

class BookingRepository {
public:
    void save(const std::shared_ptr<Booking>& booking) {
        std::lock_guard<std::mutex> lock(mutex_);
        bookings_[booking->getBookingId()] = booking;
    }

    std::shared_ptr<Booking> find(const std::string& bookingId) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = bookings_.find(bookingId);
        return it == bookings_.end() ? nullptr : it->second;
    }

private:
    std::unordered_map<std::string, std::shared_ptr<Booking>> bookings_;
    std::mutex mutex_;
};

// ---------- Payment (kept intentionally simple; see PaymentSystemDemo for
// the full idempotent/retrying payment flow) ----------

struct PaymentGateway {
    virtual ~PaymentGateway() = default;
    virtual bool charge(const std::string& userId, double amount) = 0;
};

class SimulatedPaymentGateway : public PaymentGateway {
public:
    bool charge(const std::string& userId, double amount) override {
        static thread_local std::mt19937 rng(std::random_device{}());
        static thread_local std::uniform_real_distribution<double> dist(0.0, 1.0);
        bool success = dist(rng) > 0.3;
        std::cout << ("Charging " + userId + " amount " + std::to_string(amount) + " -> " +
                      (success ? "SUCCESS" : "FAILED") + "\n");
        return success;
    }
};

// ---------- Booking service (orchestrator) ----------

class BookingSystemService {
public:
    explicit BookingSystemService(std::shared_ptr<PaymentGateway> paymentGateway,
                                   std::shared_ptr<BookingRepository> repository)
        : paymentGateway_(std::move(paymentGateway)), repository_(std::move(repository)) {}

    std::vector<std::shared_ptr<ShowSeat>> searchShow(const std::shared_ptr<Show>& show) {
        return show->findAvailableSeats();
    }

    std::shared_ptr<Booking> bookSeats(const std::shared_ptr<User>& user, const std::shared_ptr<Show>& show,
                                        std::vector<std::shared_ptr<ShowSeat>> requestedSeats) {
        auto booking = std::make_shared<Booking>(generateBookingId(), user, show, requestedSeats);

        // Step 1: atomically lock every requested seat, or fail fast if any is taken.
        if (!show->lockSeats(requestedSeats)) {
            booking->setStatus(BookingStatus::FAILED);
            std::cout << ("Booking " + booking->getBookingId() + " for " + user->getUserId() +
                          " FAILED: one or more seats already taken\n");
            return booking;
        }

        // Step 2: seats are ours exclusively now; charge the user.
        bool paid = paymentGateway_->charge(user->getUserId(), requestedSeats.size() * kPricePerSeat);

        if (paid) {
            show->markBooked(requestedSeats);
            booking->setStatus(BookingStatus::CONFIRMED);
            repository_->save(booking);
            std::string seatIds;
            for (auto& seat : requestedSeats) {
                seatIds += seat->getShowSeatId() + " ";
            }
            std::cout << ("Booking " + booking->getBookingId() + " for " + user->getUserId() +
                          " CONFIRMED for seats " + seatIds + "\n");
        } else {
            // Payment failed: release the seats back to the pool instead of
            // leaving them stuck LOCKED forever.
            show->unlockSeats(requestedSeats);
            booking->setStatus(BookingStatus::FAILED);
            std::cout << ("Booking " + booking->getBookingId() + " for " + user->getUserId() +
                          " FAILED: payment declined, seats released\n");
        }
        return booking;
    }

    bool cancelBooking(const std::string& bookingId) {
        auto booking = repository_->find(bookingId);
        if (!booking || booking->getStatus() != BookingStatus::CONFIRMED) {
            return false;
        }
        booking->getShow()->releaseBookedSeats(booking->getSeats());
        booking->setStatus(BookingStatus::CANCELLED);
        return true;
    }

private:
    static constexpr double kPricePerSeat = 250.0;

    static std::string generateBookingId() {
        static std::atomic<long long> counter{0};
        return "BK-" + std::to_string(++counter);
    }

    std::shared_ptr<PaymentGateway> paymentGateway_;
    std::shared_ptr<BookingRepository> repository_;
};

int main() {
    auto screen = std::make_shared<Screen>();
    auto show = std::make_shared<Show>(std::make_shared<Movie>("m1", "Dune Part Two"),
                                        std::chrono::system_clock::now(), std::chrono::system_clock::now(), screen);
    screen->allShows_.push_back(show);

    for (int i = 1; i <= 6; ++i) {
        auto seat = std::make_shared<Seat>("SEAT-" + std::to_string(i), SeatType::SILVER);
        show->addSeat(std::make_shared<ShowSeat>("SS-" + std::to_string(i), seat));
    }

    auto repository = std::make_shared<BookingRepository>();
    BookingSystemService bookingService(std::make_shared<SimulatedPaymentGateway>(), repository);

    // Two users race for OVERLAPPING seats {SS-2, SS-3} concurrently.
    // The atomic multi-seat lock guarantees at most one of them wins.
    std::vector<std::shared_ptr<ShowSeat>> allSeats;
    for (auto& [id, seat] : show->getShowSeats()) {
        allSeats.push_back(seat);
    }
    std::sort(allSeats.begin(), allSeats.end(),
              [](const auto& a, const auto& b) { return a->getShowSeatId() < b->getShowSeatId(); });

    std::vector<std::shared_ptr<ShowSeat>> requestA = {allSeats[0], allSeats[1], allSeats[2]}; // SS-1,2,3
    std::vector<std::shared_ptr<ShowSeat>> requestB = {allSeats[1], allSeats[2], allSeats[3]}; // SS-2,3,4

    std::shared_ptr<Booking> resultA;
    std::shared_ptr<Booking> resultB;

    std::cout << "Two users concurrently requesting overlapping seats...\n";
    std::thread t1([&]() { resultA = bookingService.bookSeats(std::make_shared<User>("alice"), show, requestA); });
    std::thread t2([&]() { resultB = bookingService.bookSeats(std::make_shared<User>("bob"), show, requestB); });
    t1.join();
    t2.join();

    int confirmed = 0;
    if (resultA && resultA->getStatus() == BookingStatus::CONFIRMED) confirmed++;
    if (resultB && resultB->getStatus() == BookingStatus::CONFIRMED) confirmed++;

    std::cout << "\nExactly one of the two overlapping requests should be CONFIRMED: confirmed count = "
              << confirmed << "\n";

    std::cout << "\nRemaining available seats: " << bookingService.searchShow(show).size() << "\n";

    return 0;
}
