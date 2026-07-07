#pragma once
#include <string>
#include <memory>

class Customer;
class Room;

// Updated documentation to enforce the business rule where Invoice is ONLY generated at the Completed stage
enum class BookingStatus {
    Upcoming,   // Reserved but not yet checked in (Invoice does not exist)
    Active,     // Currently checked in and occupying the room (Invoice does not exist)
    Completed,  // Checked out and fully settled (Triggers simultaneous Invoice generation)
    Canceled    // Reservation invalidated before check-in (Soft deletion state, no Invoice generated)
};

class Booking
{
private:
    static int bookingCounter;
    std::string bookingId;
    std::weak_ptr<Customer> customer;  // Weak reference to customer (owned by HotelManager)
    std::weak_ptr<Room> room;          // Weak reference to room (owned by HotelManager)
    std::string checkInDate;
    std::string checkOutDate;

    // Tracks the active workflow state of this specific reservation record
    BookingStatus status;

public:
    Booking();

    static void initCounterFromDatabase();

    std::string getBookingId() const;
    void setBookingId(const std::string &bookingId);

    std::shared_ptr<Customer> getCustomer() const;
    void setCustomer(const std::shared_ptr<Customer> &customer);
    void setCustomer(Customer *customer);  // Legacy overload for raw pointers

    std::shared_ptr<Room> getRoom() const;
    void setRoom(const std::shared_ptr<Room> &room);
    void setRoom(Room *room);  // Legacy overload for raw pointers

    std::string getCheckInDate() const;
    void setCheckInDate(const std::string &checkInDate);

    std::string getCheckOutDate() const;
    void setCheckOutDate(const std::string &checkOutDate);

    BookingStatus getStatus() const;
    void setStatus(BookingStatus status);
    

    // Helper utility to map enum values directly onto serialized string representations
    std::string getStatusString() const;

    bool isValid() const;  // Check if booking has valid (non-expired) customer and room
};