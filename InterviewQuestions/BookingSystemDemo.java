
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

import java.time.LocalDate;
import java.time.LocalDateTime;
import java.util.concurrent.ConcurrentHashMap;

enum ShowSeatStatus{
    AVAILABLE,
    LOCKED,
    BOOKED
}

enum BookingStatus{
    CONFIRMED,
    CANCELLED
}

class User{
    String userId;
}

class Show{
    Movie movies;
    LocalDateTime start;
    LocalDateTime endTime; 
    Screen screen;
    ConcurrentHashMap<String,ShowSeat> showSeat;


    boolean lockSeats(List<ShowSeat> seats)
        for(ShowSeat seat : seats){
            seat.status = ShowSeatStatus.LOCKED;
        }
    }

    boolean unlockSeat(List<ShowSeat> seats){
        for(ShowSeat seat : seats){
            seat.status = ShowSeatStatus.AVAILABLE;
        }
    }

    boolean markBooked(List<ShowSeat> seats){
        for(ShowSeat seat : seats){
            seat.status = ShowSeatStatus.BOOKED;
        }
    }

    boolean checkAvailability(List<ShowSeat> seats){
        for(ShowSeat seat : seats){
            if(seat.status != ShowSeatStatus.AVAILABLE){
                return false;
            }
        }
        return true;
    }
}

class Screen{
    List<Show> allShows; 
}

class Cinema{
    String cinemaId;
    String address;
    List<Screen> screens;
}

enum SeatType{
    PREMIUM,
    SILVER,
    PLATINUM
}

class Seat{
    String seatId;
    SeatType type;
}

class ShowSeat{
    String showSeatId;
    Show show;
    Seat seat;
    ShowSeatStatus status;
    
}

class Booking{
    String bookingId;
    User user;
    Show show;
    List<ShowSeat> seats; 
}

class BookingRepository{
    ConcurrentHashMap<String, Booking> bookingRepository;
}

class BookingSystem_Service{
    PaymentService service;
    BookingRepository repository;
    List<Cinema> cinemas;

    Response searchShow(String patterns){
        // use filtering librabries/ other API 
        return response;
    }


    Response bookSeats(User user, Show show, List<ShowSeat> showSeats){


        //Step-1
        boolean isAvailable = show.checkAvailability(showSeats);

        Booking booking = new Booking();
        if(isAvailable){
            //Step-2
            //Lock the seats 
            if(show.lockSeats(showSeats)){

                // Trigger the payment
                Request request = new Request(); 
                service.createPayment(null)
                
                // if payment is successful
                show.markBooked(showSeats);
                booking.status = BookingStatus.CONFIRMED;

                
                // If failed payment 
                show.unlockSeat(Seats);


            }else{
                //handle failure 
            }


        }else{

            // seats not avaible don't do anything

        }





    }



}






public class BookingSystemDemo {
    
}
