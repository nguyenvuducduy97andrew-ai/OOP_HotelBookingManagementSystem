#pragma once
#include <string>
#include <memory>

class Customer;
class Room;
class HotelManager;
class BookingService;

enum class BookingState { UPCOMING, ACTIVE, COMPLETED, CANCELLED };

class Booking
{
private:
    static int bookingCounter;
    std::string bookingId;
    std::weak_ptr<Customer> customer;  // Weak reference to customer (owned by HotelManager)
    std::weak_ptr<Room> room;          // Weak reference to room (owned by HotelManager)
    std::string checkInDate;
    std::string checkOutDate;
    bool cancelled; 
    bool deleted;
    // Modified and optimized performance: persist the explicit checkout event independently from planned stay dates.
    bool checkedOut;

public:
    Booking();

    // Modified and optimized performance: restrict booking-ID assignment to the manager facade and controlled booking workflow.
    friend class HotelManager;
    friend class BookingService;

    static std::string nextBookingId();
    static void initCounterFromDatabase(int maxBookingNumber);

    std::string getBookingId() const;
    
private:
    // Fixed-modified: Keep booking IDs write-protected outside the manager layer.
    void setBookingId(const std::string &bookingId);

public:

    std::shared_ptr<Customer> getCustomer() const;
    void setCustomer(const std::shared_ptr<Customer> &customer);

    std::shared_ptr<Room> getRoom() const;
    void setRoom(const std::shared_ptr<Room> &room);

    std::string getCheckInDate() const;
    void setCheckInDate(const std::string &checkInDate);

    std::string getCheckOutDate() const;
    void setCheckOutDate(const std::string &checkOutDate);

    bool isCancelled() const;
    void setCancelled(bool cancelled);

    bool isDeleted() const;
    void setDeleted(bool deleted);

    // Modified and optimized performance: expose the persisted checkout state used by operational and history views.
    bool isCheckedOut() const;
    void setCheckedOut(bool checkedOut);

    bool isValid() const;  // Check if booking has valid (non-expired) customer and room
};
