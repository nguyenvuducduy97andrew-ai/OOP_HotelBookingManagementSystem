#include "Booking.h"
#include "Customer.h"
#include "Room.h"

#include <string>

//The booking counter started at 1000 to be used as BookingID
int Booking::bookingCounter = 1000;
Booking::Booking() {
    bookingCounter++;
    this->bookingId = "BK" + std::to_string(bookingCounter);
    this->checkInDate = QDate::currentDate();
    this->checkOutDate = QDate::currentDate().addDays(2);
}

std::string Booking::getBookingId() const{
    return bookingId;
}
void Booking::setBookingId(const std::string& bookingId) {
    this->bookingId = bookingId;
}

std::shared_ptr<Customer> Booking::getCustomer() const {
    // Lock the weak_ptr to get a shared_ptr; returns nullptr if expired
    return customer.lock();
}
void Booking::setCustomer(const std::shared_ptr<Customer>& customer) {
    // Store weak reference (does not increase refcount)
    this->customer = customer;
}

void Booking::setCustomer(Customer* customer) {
    // Legacy raw pointer overload - cannot create weak_ptr from raw pointer safely
    this->customer = std::weak_ptr<Customer>();
}

std::shared_ptr<Room> Booking::getRoom() const {
    // Lock the weak_ptr to get a shared_ptr; returns nullptr if expired
    return room.lock();
}
void Booking::setRoom(const std::shared_ptr<Room>& room) {
    // Store weak reference (does not increase refcount)
    this->room = room;
}

void Booking::setRoom(Room* room) {
    // Legacy raw pointer overload - cannot create weak_ptr from raw pointer safely
    this->room = std::weak_ptr<Room>();
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

bool Booking::isValid() const {
    // Check if both weak_ptrs are still valid (not expired)
    auto lockedCustomer = customer.lock();
    auto lockedRoom = room.lock();

    return lockedCustomer != nullptr && lockedRoom != nullptr &&
           checkInDate.isValid() && checkOutDate.isValid() &&
           checkOutDate > checkInDate;
}