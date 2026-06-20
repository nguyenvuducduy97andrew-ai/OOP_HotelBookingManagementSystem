#pragma once
#include <string>
#include <QDate>


class Customer;
class Room;

class Booking
{
private:
    static int bookingCounter;
    std::string bookingId;
    Customer *customer;
    Room *room;
    QDate checkInDate;
    QDate checkOutDate;

public:
    Booking();

    std::string getBookingId() const;
    void setBookingId(const std::string &bookingId);

    Customer *getCustomer() const;
    void setCustomer(Customer *customer);

    Room *getRoom() const;
    void setRoom(Room *room);

    QDate getCheckInDate() const;
    void setCheckInDate(const QDate &checkInDate);

    QDate getCheckOutDate() const;
    void setCheckOutDate(const QDate &checkOutDate);

    int getDurationInNights() const;
};

