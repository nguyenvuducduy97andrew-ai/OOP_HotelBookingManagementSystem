#include "Booking.h"
#include "Customer.h"
#include "Room.h"

#include <string>

//The booking counter started at 1000 to be used as BookingID
int Booking::bookingCounter = 1000;
Booking::Booking() {
    bookingCounter++;
    this->bookingId = "BK" + std::to_string(bookingCounter);
    this->customer = nullptr;
    this->room = nullptr;
    this->checkInDate = QDate::currentDate();
    this->checkOutDate = QDate::currentDate().addDays(2);
}

std::string Booking::getBookingId() const{
    return bookingId;
}
void Booking::setBookingId(const std::string& bookingId) {
    this->bookingId = bookingId;
}

Customer* Booking::getCustomer() const {
    return customer;
}
void Booking::setCustomer(Customer* customer) {
    this->customer = customer;
}

Room* Booking::getRoom() const {
    return room;
}
void Booking::setRoom(Room* room) {
    this->room = room;
}

QDate Booking::getCheckInDate() const {
    return checkInDate;
}
void Booking::setCheckInDate(const QDate& checkInDate) {
    this->checkInDate = checkInDate;
}

QDate Booking::getCheckOutDate() const {
    return checkOutDate;
}
void Booking::setCheckOutDate(const QDate& checkOutDate) {
    this->checkOutDate = checkOutDate;
}

int Booking::getDurationInNights() const {
    if (!checkInDate.isValid() || !checkOutDate.isValid()) return 0;
    if (checkOutDate < checkInDate) return 0;
    return checkInDate.daysTo(checkOutDate);
}