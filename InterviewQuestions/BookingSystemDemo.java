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

import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.ReentrantLock;

// ---------- Enums ----------

enum ShowSeatStatus {
    AVAILABLE, LOCKED, BOOKED
}

enum BookingStatus {
    PENDING, CONFIRMED, CANCELLED, FAILED
}

enum SeatType {
    PREMIUM, SILVER, PLATINUM
}

// ---------- Core entities ----------

class User {
    private final String userId;

    public User(String userId) {
        this.userId = userId;
    }

    public String getUserId() {
        return userId;
    }
}

class Movie {
    private final String movieId;
    private final String title;

    public Movie(String movieId, String title) {
        this.movieId = movieId;
        this.title = title;
    }

    public String getTitle() {
        return title;
    }
}

class Seat {
    private final String seatId;
    private final SeatType type;

    public Seat(String seatId, SeatType type) {
        this.seatId = seatId;
        this.type = type;
    }

    public String getSeatId() {
        return seatId;
    }

    public SeatType getType() {
        return type;
    }
}

// One seat for one specific show. Its own lock guards status transitions;
// multi-seat operations acquire these locks in a globally consistent order
// (sorted by showSeatId) so concurrent bookings can never deadlock on them.
class ShowSeat {
    private final String showSeatId;
    private final Seat seat;
    private volatile ShowSeatStatus status = ShowSeatStatus.AVAILABLE;
    private final ReentrantLock lock = new ReentrantLock();

    public ShowSeat(String showSeatId, Seat seat) {
        this.showSeatId = showSeatId;
        this.seat = seat;
    }

    public String getShowSeatId() {
        return showSeatId;
    }

    public Seat getSeat() {
        return seat;
    }

    public ShowSeatStatus getStatus() {
        return status;
    }

    ReentrantLock getLock() {
        return lock;
    }

    void setStatus(ShowSeatStatus status) {
        this.status = status;
    }
}

class Screen {
    private final List<Show> allShows = new ArrayList<>();

    public List<Show> getAllShows() {
        return allShows;
    }
}

class Show {
    private final Movie movie;
    private final LocalDateTime start;
    private final LocalDateTime endTime;
    private final Screen screen;
    private final Map<String, ShowSeat> showSeats = new ConcurrentHashMap<>();

    public Show(Movie movie, LocalDateTime start, LocalDateTime endTime, Screen screen) {
        this.movie = movie;
        this.start = start;
        this.endTime = endTime;
        this.screen = screen;
    }

    public Movie getMovie() {
        return movie;
    }

    public void addSeat(ShowSeat seat) {
        showSeats.put(seat.getShowSeatId(), seat);
    }

    public Map<String, ShowSeat> getShowSeats() {
        return showSeats;
    }

    // Atomically locks every seat in `seats`, or none of them.
    //
    // Deadlock avoidance: two concurrent calls that both touch seats {A, B}
    // (regardless of the caller's order) always acquire A before B, because
    // we sort by showSeatId first. That total lock order rules out the
    // classic "thread 1 holds A wants B, thread 2 holds B wants A" cycle.
    public boolean lockSeats(List<ShowSeat> seats) {
        List<ShowSeat> sorted = new ArrayList<>(seats);
        sorted.sort(Comparator.comparing(ShowSeat::getShowSeatId));

        List<ShowSeat> acquired = new ArrayList<>();
        try {
            for (ShowSeat seat : sorted) {
                seat.getLock().lock();
                acquired.add(seat);
            }
            for (ShowSeat seat : sorted) {
                if (seat.getStatus() != ShowSeatStatus.AVAILABLE) {
                    return false; // bail out; finally block below releases everything
                }
            }
            for (ShowSeat seat : sorted) {
                seat.setStatus(ShowSeatStatus.LOCKED);
            }
            return true;
        } finally {
            for (ShowSeat seat : acquired) {
                seat.getLock().unlock();
            }
        }
    }

    public void unlockSeats(List<ShowSeat> seats) {
        for (ShowSeat seat : seats) {
            seat.getLock().lock();
            try {
                if (seat.getStatus() == ShowSeatStatus.LOCKED) {
                    seat.setStatus(ShowSeatStatus.AVAILABLE);
                }
            } finally {
                seat.getLock().unlock();
            }
        }
    }

    public void markBooked(List<ShowSeat> seats) {
        for (ShowSeat seat : seats) {
            seat.getLock().lock();
            try {
                seat.setStatus(ShowSeatStatus.BOOKED);
            } finally {
                seat.getLock().unlock();
            }
        }
    }

    // Cancellation: BOOKED seats go back to AVAILABLE for resale.
    public void releaseBookedSeats(List<ShowSeat> seats) {
        for (ShowSeat seat : seats) {
            seat.getLock().lock();
            try {
                if (seat.getStatus() == ShowSeatStatus.BOOKED) {
                    seat.setStatus(ShowSeatStatus.AVAILABLE);
                }
            } finally {
                seat.getLock().unlock();
            }
        }
    }

    public List<ShowSeat> findAvailableSeats() {
        List<ShowSeat> available = new ArrayList<>();
        for (ShowSeat seat : showSeats.values()) {
            if (seat.getStatus() == ShowSeatStatus.AVAILABLE) {
                available.add(seat);
            }
        }
        return available;
    }
}

class Cinema {
    private final String cinemaId;
    private final String address;
    private final List<Screen> screens = new ArrayList<>();

    public Cinema(String cinemaId, String address) {
        this.cinemaId = cinemaId;
        this.address = address;
    }

    public List<Screen> getScreens() {
        return screens;
    }
}

class Booking {
    private final String bookingId;
    private final User user;
    private final Show show;
    private final List<ShowSeat> seats;
    private volatile BookingStatus status;

    public Booking(String bookingId, User user, Show show, List<ShowSeat> seats) {
        this.bookingId = bookingId;
        this.user = user;
        this.show = show;
        this.seats = seats;
        this.status = BookingStatus.PENDING;
    }

    public String getBookingId() {
        return bookingId;
    }

    public Show getShow() {
        return show;
    }

    public List<ShowSeat> getSeats() {
        return seats;
    }

    public BookingStatus getStatus() {
        return status;
    }

    public void setStatus(BookingStatus status) {
        this.status = status;
    }
}

class BookingRepository {
    private final Map<String, Booking> bookings = new ConcurrentHashMap<>();

    public void save(Booking booking) {
        bookings.put(booking.getBookingId(), booking);
    }

    public Optional<Booking> find(String bookingId) {
        return Optional.ofNullable(bookings.get(bookingId));
    }
}

// ---------- Payment (kept intentionally simple; see PaymentSystemDemo for
// the full idempotent/retrying payment flow) ----------

interface PaymentGateway {
    boolean charge(String userId, double amount);
}

class SimulatedPaymentGateway implements PaymentGateway {
    private final java.util.Random random = new java.util.Random();

    @Override
    public boolean charge(String userId, double amount) {
        boolean success = random.nextDouble() > 0.3;
        System.out.println("Charging " + userId + " amount " + amount + " -> " + (success ? "SUCCESS" : "FAILED"));
        return success;
    }
}

// ---------- Booking service (orchestrator) ----------

class BookingSystem_Service {
    private final PaymentGateway paymentGateway;
    private final BookingRepository repository;
    private static final double PRICE_PER_SEAT = 250.0;

    public BookingSystem_Service(PaymentGateway paymentGateway, BookingRepository repository) {
        this.paymentGateway = paymentGateway;
        this.repository = repository;
    }

    public List<ShowSeat> searchShow(Show show) {
        return show.findAvailableSeats();
    }

    public Booking bookSeats(User user, Show show, List<ShowSeat> requestedSeats) {
        Booking booking = new Booking(UUID.randomUUID().toString(), user, show, requestedSeats);

        // Step 1: atomically lock every requested seat, or fail fast if any is taken.
        if (!show.lockSeats(requestedSeats)) {
            booking.setStatus(BookingStatus.FAILED);
            System.out.println("Booking " + booking.getBookingId() + " for " + user.getUserId()
                    + " FAILED: one or more seats already taken");
            return booking;
        }

        // Step 2: seats are ours exclusively now; charge the user.
        boolean paid = paymentGateway.charge(user.getUserId(), requestedSeats.size() * PRICE_PER_SEAT);

        if (paid) {
            show.markBooked(requestedSeats);
            booking.setStatus(BookingStatus.CONFIRMED);
            repository.save(booking);
            System.out.println("Booking " + booking.getBookingId() + " for " + user.getUserId()
                    + " CONFIRMED for seats " + seatIds(requestedSeats));
        } else {
            // Payment failed: release the seats back to the pool instead of
            // leaving them stuck LOCKED forever.
            show.unlockSeats(requestedSeats);
            booking.setStatus(BookingStatus.FAILED);
            System.out.println("Booking " + booking.getBookingId() + " for " + user.getUserId()
                    + " FAILED: payment declined, seats released");
        }
        return booking;
    }

    public boolean cancelBooking(String bookingId) {
        Optional<Booking> maybeBooking = repository.find(bookingId);
        if (maybeBooking.isEmpty() || maybeBooking.get().getStatus() != BookingStatus.CONFIRMED) {
            return false;
        }
        Booking booking = maybeBooking.get();
        booking.getShow().releaseBookedSeats(booking.getSeats());
        booking.setStatus(BookingStatus.CANCELLED);
        return true;
    }

    private static String seatIds(List<ShowSeat> seats) {
        StringBuilder sb = new StringBuilder();
        for (ShowSeat seat : seats) {
            sb.append(seat.getShowSeatId()).append(' ');
        }
        return sb.toString().trim();
    }
}

// ---------- Demo / Simulation ----------

public class BookingSystemDemo {

    public static void main(String[] args) throws InterruptedException {
        Screen screen = new Screen();
        Show show = new Show(new Movie("m1", "Dune Part Two"), LocalDateTime.now(), LocalDateTime.now().plusHours(2), screen);
        screen.getAllShows().add(show);

        for (int i = 1; i <= 6; i++) {
            Seat seat = new Seat("SEAT-" + i, SeatType.SILVER);
            show.addSeat(new ShowSeat("SS-" + i, seat));
        }

        BookingRepository repository = new BookingRepository();
        BookingSystem_Service bookingService = new BookingSystem_Service(new SimulatedPaymentGateway(), repository);

        // Two users race for OVERLAPPING seats {SS-2, SS-3} concurrently.
        // The atomic multi-seat lock guarantees at most one of them wins.
        List<ShowSeat> allSeats = new ArrayList<>(show.getShowSeats().values());
        allSeats.sort(Comparator.comparing(ShowSeat::getShowSeatId));

        List<ShowSeat> requestA = List.of(allSeats.get(0), allSeats.get(1), allSeats.get(2)); // SS-1, SS-2, SS-3
        List<ShowSeat> requestB = List.of(allSeats.get(1), allSeats.get(2), allSeats.get(3)); // SS-2, SS-3, SS-4

        ExecutorService executor = Executors.newFixedThreadPool(2);
        CountDownLatch latch = new CountDownLatch(2);
        Booking[] results = new Booking[2];

        System.out.println("Two users concurrently requesting overlapping seats...");
        executor.submit(() -> {
            try {
                results[0] = bookingService.bookSeats(new User("alice"), show, requestA);
            } finally {
                latch.countDown();
            }
        });
        executor.submit(() -> {
            try {
                results[1] = bookingService.bookSeats(new User("bob"), show, requestB);
            } finally {
                latch.countDown();
            }
        });

        latch.await();
        executor.shutdown();
        executor.awaitTermination(5, TimeUnit.SECONDS);

        long confirmed = 0;
        for (Booking booking : results) {
            if (booking.getStatus() == BookingStatus.CONFIRMED) {
                confirmed++;
            }
        }
        System.out.println("\nExactly one of the two overlapping requests should be CONFIRMED: confirmed count = " + confirmed);

        System.out.println("\nRemaining available seats: " + bookingService.searchShow(show).size());
    }
}
