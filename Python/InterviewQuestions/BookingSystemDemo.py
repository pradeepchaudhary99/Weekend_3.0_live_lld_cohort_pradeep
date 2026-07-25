"""
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

    search_show(pattern)
    book_seats(user, show, seats)
"""

from __future__ import annotations

import random
import threading
import uuid
from datetime import datetime
from enum import Enum, auto
from typing import Dict, List, Optional


class ShowSeatStatus(Enum):
    AVAILABLE = auto()
    LOCKED = auto()
    BOOKED = auto()


class BookingStatus(Enum):
    PENDING = auto()
    CONFIRMED = auto()
    CANCELLED = auto()
    FAILED = auto()


class SeatType(Enum):
    PREMIUM = auto()
    SILVER = auto()
    PLATINUM = auto()


class User:
    def __init__(self, user_id: str):
        self.user_id = user_id


class Movie:
    def __init__(self, movie_id: str, title: str):
        self.movie_id = movie_id
        self.title = title


class Seat:
    def __init__(self, seat_id: str, seat_type: SeatType):
        self.seat_id = seat_id
        self.type = seat_type


class ShowSeat:
    """One seat for one specific show. Its own lock guards status transitions;
    multi-seat operations acquire these locks in a globally consistent order
    (sorted by show_seat_id) so concurrent bookings can never deadlock on them."""

    def __init__(self, show_seat_id: str, seat: Seat):
        self.show_seat_id = show_seat_id
        self.seat = seat
        self.status = ShowSeatStatus.AVAILABLE
        self.lock = threading.Lock()


class Screen:
    def __init__(self):
        self.all_shows: List["Show"] = []


class Show:
    def __init__(self, movie: Movie, start: datetime, end_time: datetime, screen: Screen):
        self.movie = movie
        self.start = start
        self.end_time = end_time
        self.screen = screen
        self.show_seats: Dict[str, ShowSeat] = {}

    def add_seat(self, seat: ShowSeat) -> None:
        self.show_seats[seat.show_seat_id] = seat

    def lock_seats(self, seats: List[ShowSeat]) -> bool:
        """Atomically locks every seat in `seats`, or none of them.

        Deadlock avoidance: two concurrent calls that both touch seats {A, B}
        (regardless of the caller's order) always acquire A before B, because
        we sort by show_seat_id first. That total lock order rules out the
        classic "thread 1 holds A wants B, thread 2 holds B wants A" cycle.
        """
        ordered = sorted(seats, key=lambda s: s.show_seat_id)
        acquired: List[ShowSeat] = []
        try:
            for seat in ordered:
                seat.lock.acquire()
                acquired.append(seat)

            if any(seat.status != ShowSeatStatus.AVAILABLE for seat in ordered):
                return False

            for seat in ordered:
                seat.status = ShowSeatStatus.LOCKED
            return True
        finally:
            for seat in acquired:
                seat.lock.release()

    def unlock_seats(self, seats: List[ShowSeat]) -> None:
        for seat in seats:
            with seat.lock:
                if seat.status == ShowSeatStatus.LOCKED:
                    seat.status = ShowSeatStatus.AVAILABLE

    def mark_booked(self, seats: List[ShowSeat]) -> None:
        for seat in seats:
            with seat.lock:
                seat.status = ShowSeatStatus.BOOKED

    def release_booked_seats(self, seats: List[ShowSeat]) -> None:
        """Cancellation: BOOKED seats go back to AVAILABLE for resale."""
        for seat in seats:
            with seat.lock:
                if seat.status == ShowSeatStatus.BOOKED:
                    seat.status = ShowSeatStatus.AVAILABLE

    def find_available_seats(self) -> List[ShowSeat]:
        return [seat for seat in self.show_seats.values() if seat.status == ShowSeatStatus.AVAILABLE]


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
        self.status: BookingStatus = BookingStatus.PENDING


class BookingRepository:
    def __init__(self):
        self._bookings: Dict[str, Booking] = {}
        self._lock = threading.Lock()

    def save(self, booking: Booking) -> None:
        with self._lock:
            self._bookings[booking.booking_id] = booking

    def find(self, booking_id: str) -> Optional[Booking]:
        with self._lock:
            return self._bookings.get(booking_id)


# ---------- Payment (kept intentionally simple; see PaymentSystemDemo for
# the full idempotent/retrying payment flow) ----------

class PaymentGateway:
    def charge(self, user_id: str, amount: float) -> bool:
        raise NotImplementedError


class SimulatedPaymentGateway(PaymentGateway):
    def charge(self, user_id: str, amount: float) -> bool:
        success = random.random() > 0.3
        print(f"Charging {user_id} amount {amount} -> {'SUCCESS' if success else 'FAILED'}")
        return success


class BookingSystemService:
    PRICE_PER_SEAT = 250.0

    def __init__(self, payment_gateway: PaymentGateway, repository: BookingRepository):
        self._payment_gateway = payment_gateway
        self._repository = repository

    def search_show(self, show: Show) -> List[ShowSeat]:
        return show.find_available_seats()

    def book_seats(self, user: User, show: Show, requested_seats: List[ShowSeat]) -> Booking:
        booking = Booking(str(uuid.uuid4()), user, show, requested_seats)

        # Step 1: atomically lock every requested seat, or fail fast if any is taken.
        if not show.lock_seats(requested_seats):
            booking.status = BookingStatus.FAILED
            print(f"Booking {booking.booking_id} for {user.user_id} FAILED: "
                  f"one or more seats already taken")
            return booking

        # Step 2: seats are ours exclusively now; charge the user.
        paid = self._payment_gateway.charge(user.user_id, len(requested_seats) * self.PRICE_PER_SEAT)

        if paid:
            show.mark_booked(requested_seats)
            booking.status = BookingStatus.CONFIRMED
            self._repository.save(booking)
            seat_ids = " ".join(s.show_seat_id for s in requested_seats)
            print(f"Booking {booking.booking_id} for {user.user_id} CONFIRMED for seats {seat_ids}")
        else:
            # Payment failed: release the seats back to the pool instead of
            # leaving them stuck LOCKED forever.
            show.unlock_seats(requested_seats)
            booking.status = BookingStatus.FAILED
            print(f"Booking {booking.booking_id} for {user.user_id} FAILED: "
                  f"payment declined, seats released")

        return booking

    def cancel_booking(self, booking_id: str) -> bool:
        booking = self._repository.find(booking_id)
        if booking is None or booking.status != BookingStatus.CONFIRMED:
            return False
        booking.show.release_booked_seats(booking.seats)
        booking.status = BookingStatus.CANCELLED
        return True


def main() -> None:
    screen = Screen()
    show = Show(Movie("m1", "Dune Part Two"), datetime.now(), datetime.now(), screen)
    screen.all_shows.append(show)

    for i in range(1, 7):
        seat = Seat(f"SEAT-{i}", SeatType.SILVER)
        show.add_seat(ShowSeat(f"SS-{i}", seat))

    repository = BookingRepository()
    booking_service = BookingSystemService(SimulatedPaymentGateway(), repository)

    # Two users race for OVERLAPPING seats {SS-2, SS-3} concurrently.
    # The atomic multi-seat lock guarantees at most one of them wins.
    all_seats = sorted(show.show_seats.values(), key=lambda s: s.show_seat_id)
    request_a = [all_seats[0], all_seats[1], all_seats[2]]  # SS-1, SS-2, SS-3
    request_b = [all_seats[1], all_seats[2], all_seats[3]]  # SS-2, SS-3, SS-4

    results: List[Optional[Booking]] = [None, None]

    def book_a() -> None:
        results[0] = booking_service.book_seats(User("alice"), show, request_a)

    def book_b() -> None:
        results[1] = booking_service.book_seats(User("bob"), show, request_b)

    print("Two users concurrently requesting overlapping seats...")
    t1 = threading.Thread(target=book_a)
    t2 = threading.Thread(target=book_b)
    t1.start()
    t2.start()
    t1.join()
    t2.join()

    confirmed = sum(1 for booking in results if booking is not None and booking.status == BookingStatus.CONFIRMED)
    print(f"\nExactly one of the two overlapping requests should be CONFIRMED: confirmed count = {confirmed}")

    print(f"\nRemaining available seats: {len(booking_service.search_show(show))}")


if __name__ == "__main__":
    main()
