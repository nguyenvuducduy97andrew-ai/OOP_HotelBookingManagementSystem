#pragma once
#include <string>
#include <memory>
#include <QDate>

class Customer;
class Room;

class Booking
{
private:
    static int bookingCounter;
    std::string bookingId;
    std::weak_ptr<Customer> customer;  // Weak reference to customer (owned by HotelManager)
    std::weak_ptr<Room> room;          // Weak reference to room (owned by HotelManager)
    QDate checkInDate;
    QDate checkOutDate;

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

    QDate getCheckInDate() const;
    void setCheckInDate(const QDate &checkInDate);

    QDate getCheckOutDate() const;
    void setCheckOutDate(const QDate &checkOutDate);

    int getDurationInNights() const;
    bool isValid() const;  // Check if booking has valid (non-expired) customer and room
};

