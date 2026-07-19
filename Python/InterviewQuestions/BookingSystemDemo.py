"""
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
"""

from datetime import datetime
from enum import Enum, auto
from typing import Dict, List, Optional


class ShowSeatStatus(Enum):
    AVAILABLE = auto()
    LOCKED = auto()
    BOOKED = auto()


class BookingStatus(Enum):
    CONFIRMED = auto()
    CANCELLED = auto()


class User:
    def __init__(self, user_id: str):
        self.user_id = user_id


class Movie:
    def __init__(self, movie_id: str, title: str):
        self.movie_id = movie_id
        self.title = title


class SeatType(Enum):
    PREMIUM = auto()
    SILVER = auto()
    PLATINUM = auto()


class Seat:
    def __init__(self, seat_id: str, seat_type: SeatType):
        self.seat_id = seat_id
        self.type = seat_type


class ShowSeat:
    def __init__(self, show_seat_id: str, show: "Show", seat: Seat):
        self.show_seat_id = show_seat_id
        self.show = show
        self.seat = seat
        self.status = ShowSeatStatus.AVAILABLE


class Show:
    def __init__(self, movie: Movie, start: datetime, end_time: datetime, screen: "Screen"):
        self.movie = movie
        self.start = start
        self.end_time = end_time
        self.screen = screen
        self.show_seats: Dict[str, ShowSeat] = {}

    def lock_seats(self, seats: List[ShowSeat]) -> bool:
        for seat in seats:
            seat.status = ShowSeatStatus.LOCKED
        return True

    def unlock_seats(self, seats: List[ShowSeat]) -> bool:
        for seat in seats:
            seat.status = ShowSeatStatus.AVAILABLE
        return True

    def mark_booked(self, seats: List[ShowSeat]) -> bool:
        for seat in seats:
            seat.status = ShowSeatStatus.BOOKED
        return True

    def check_availability(self, seats: List[ShowSeat]) -> bool:
        for seat in seats:
            if seat.status != ShowSeatStatus.AVAILABLE:
                return False
        return True


class Screen:
    def __init__(self):
        self.all_shows: List[Show] = []


class Cinema:
    def __init__(self, cinema_id: str, address: str):
        self.cinema_id = cinema_id
        self.address = address
        self.screens: List[Screen] = []


class Booking:
    def __init__(self, booking_id: str, user: User, show: Show, seats: List[ShowSeat]):
        self.booking_id = booking_id
        self.user = user
        self.show = show
        self.seats = seats
        self.status: Optional[BookingStatus] = None


class BookingRepository:
    def __init__(self):
        self.bookings: Dict[str, Booking] = {}


class PaymentService:
    def create_payment(self, request) -> None:
        pass


class BookingSystem_Service:
    def __init__(self, payment_service: PaymentService, repository: BookingRepository):
        self.payment_service = payment_service
        self.repository = repository
        self.cinemas: List[Cinema] = []

    def search_show(self, pattern: str):
        # use filtering libraries/ other API
        pass

    def book_seats(self, user: User, show: Show, show_seats: List[ShowSeat]) -> Booking:
        # Step-1
        is_available = show.check_availability(show_seats)

        booking = Booking("", user, show, show_seats)
        if is_available:
            # Step-2
            # Lock the seats
            if show.lock_seats(show_seats):
                # Trigger the payment
                self.payment_service.create_payment(None)

                # if payment is successful
                show.mark_booked(show_seats)
                booking.status = BookingStatus.CONFIRMED

                # If failed payment
                # show.unlock_seats(show_seats)
            else:
                # handle failure
                pass
        else:
            # seats not available don't do anything
            pass

        return booking


def main() -> None:
    pass


if __name__ == "__main__":
    main()
