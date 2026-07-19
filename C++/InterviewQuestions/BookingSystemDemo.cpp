/*
TicketMaster
BookMyShow
HotelBookingSystem


// 20% part will give 80% result
1. Idempotency
2. Avoid Double Booking //
3. Seat Locking


FR:
    User should be able to search for available seats
    User should be able to book one or more seats
    User can cancel a booking
    Prevnt double booking of the same seat
    Support multiple payment method


NFR:




Entities:

User
Seat
Screen/Audi
Cinema
Show
movie
ShowSeat

Booking

BookingStatus

ShowSeatStatus
available
Locked
Booked

BookingService
    --> BookingRepository
    --> ShowRepository

    searchShow(showId, //pattern, city) // --->
    createBooking(showId, List<ShowSeat>, userId)
*/

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

enum class ShowSeatStatus { AVAILABLE, LOCKED, BOOKED };

enum class BookingStatus { CONFIRMED, CANCELLED };

class User {
public:
    explicit User(std::string userId) : userId_(std::move(userId)) {}
    std::string userId_;
};

class Movie {
public:
    Movie(std::string movieId, std::string title)
        : movieId_(std::move(movieId)), title_(std::move(title)) {}
    std::string movieId_;
    std::string title_;
};

enum class SeatType { PREMIUM, SILVER, PLATINUM };

class Seat {
public:
    Seat(std::string seatId, SeatType type) : seatId_(std::move(seatId)), type_(type) {}
    std::string seatId_;
    SeatType type_;
};

class Show;

class ShowSeat {
public:
    ShowSeat(std::string showSeatId, Show* show, std::shared_ptr<Seat> seat)
        : showSeatId_(std::move(showSeatId)), show_(show), seat_(std::move(seat)),
          status_(ShowSeatStatus::AVAILABLE) {}

    std::string showSeatId_;
    Show* show_;
    std::shared_ptr<Seat> seat_;
    ShowSeatStatus status_;
};

class Screen {
public:
    std::vector<std::shared_ptr<Show>> allShows_;
};

class Show {
public:
    Show(std::shared_ptr<Movie> movie, std::chrono::system_clock::time_point start,
         std::chrono::system_clock::time_point endTime, std::shared_ptr<Screen> screen)
        : movie_(std::move(movie)), start_(start), endTime_(endTime), screen_(std::move(screen)) {}

    bool lockSeats(std::vector<std::shared_ptr<ShowSeat>>& seats) {
        for (auto& seat : seats) {
            seat->status_ = ShowSeatStatus::LOCKED;
        }
        return true;
    }

    bool unlockSeats(std::vector<std::shared_ptr<ShowSeat>>& seats) {
        for (auto& seat : seats) {
            seat->status_ = ShowSeatStatus::AVAILABLE;
        }
        return true;
    }

    bool markBooked(std::vector<std::shared_ptr<ShowSeat>>& seats) {
        for (auto& seat : seats) {
            seat->status_ = ShowSeatStatus::BOOKED;
        }
        return true;
    }

    bool checkAvailability(const std::vector<std::shared_ptr<ShowSeat>>& seats) const {
        for (const auto& seat : seats) {
            if (seat->status_ != ShowSeatStatus::AVAILABLE) {
                return false;
            }
        }
        return true;
    }

    std::shared_ptr<Movie> movie_;
    std::chrono::system_clock::time_point start_;
    std::chrono::system_clock::time_point endTime_;
    std::shared_ptr<Screen> screen_;
    std::unordered_map<std::string, std::shared_ptr<ShowSeat>> showSeats_;
};

class Cinema {
public:
    Cinema(std::string cinemaId, std::string address)
        : cinemaId_(std::move(cinemaId)), address_(std::move(address)) {}
    std::string cinemaId_;
    std::string address_;
    std::vector<std::shared_ptr<Screen>> screens_;
};

class Booking {
public:
    Booking(std::string bookingId, std::shared_ptr<User> user, std::shared_ptr<Show> show,
            std::vector<std::shared_ptr<ShowSeat>> seats)
        : bookingId_(std::move(bookingId)), user_(std::move(user)), show_(std::move(show)),
          seats_(std::move(seats)) {}

    std::string bookingId_;
    std::shared_ptr<User> user_;
    std::shared_ptr<Show> show_;
    std::vector<std::shared_ptr<ShowSeat>> seats_;
    BookingStatus status_ = BookingStatus::CANCELLED;
};

class BookingRepository {
public:
    std::unordered_map<std::string, std::shared_ptr<Booking>> bookings_;
};

class PaymentService {
public:
    virtual ~PaymentService() = default;
    virtual void createPayment(void* request) {}
};

class BookingSystem_Service {
public:
    BookingSystem_Service(std::shared_ptr<PaymentService> paymentService,
                           std::shared_ptr<BookingRepository> repository)
        : paymentService_(std::move(paymentService)), repository_(std::move(repository)) {}

    void searchShow(const std::string& pattern) {
        // use filtering libraries/ other API
    }

    std::shared_ptr<Booking> bookSeats(const std::shared_ptr<User>& user,
                                        const std::shared_ptr<Show>& show,
                                        std::vector<std::shared_ptr<ShowSeat>> showSeats) {
        // Step-1
        bool isAvailable = show->checkAvailability(showSeats);

        auto booking = std::make_shared<Booking>("", user, show, showSeats);
        if (isAvailable) {
            // Step-2
            // Lock the seats
            if (show->lockSeats(showSeats)) {
                // Trigger the payment
                paymentService_->createPayment(nullptr);

                // if payment is successful
                show->markBooked(showSeats);
                booking->status_ = BookingStatus::CONFIRMED;

                // If failed payment
                // show->unlockSeats(showSeats);
            } else {
                // handle failure
            }
        } else {
            // seats not available don't do anything
        }

        return booking;
    }

private:
    std::shared_ptr<PaymentService> paymentService_;
    std::shared_ptr<BookingRepository> repository_;
    std::vector<std::shared_ptr<Cinema>> cinemas_;
};

int main() {
    return 0;
}
