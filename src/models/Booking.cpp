#include "Booking.h"
#include "Customer.h"
#include "Room.h"
#include <string>
#include <iomanip>
#include <sstream>

namespace {
// Fixed-modified: Replace Qt date checks with standard C++ ISO parsing for booking validity.
bool isIsoDateString(const std::string& value)
{
    std::tm parsed{};
    std::istringstream input(value);
    input >> std::get_time(&parsed, "%Y-%m-%d");
    return !input.fail() && input.eof();
}
}

//The booking counter started at 1000 to be used as BookingID
int Booking::bookingCounter = 1000;

Booking::Booking() {
    // Fixed-modified: Keep booking construction lightweight and assign the ID from HotelManager instead.
    this->cancelled = false;
    this->deleted = false;
    this->checkOutDate = "";
    this->checkInDate = "";
}

std::string Booking::nextBookingId() {
    // Fixed-modified: Generate booking IDs only when the manager commits a booking.
    bookingCounter++;
    return "BK" + std::to_string(bookingCounter);
}

// Restore the booking counter from the highest persisted booking number.
void Booking::initCounterFromDatabase(int maxBookingNumber) {
    if (maxBookingNumber > bookingCounter) {
        bookingCounter = maxBookingNumber;
    }
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

std::shared_ptr<Room> Booking::getRoom() const {
    // Lock the weak_ptr to get a shared_ptr; returns nullptr if expired
    return room.lock();
}
void Booking::setRoom(const std::shared_ptr<Room>& room) {
    // Store weak reference (does not increase refcount)
    this->room = room;
}

std::string Booking::getCheckInDate() const {
    return checkInDate;
}
void Booking::setCheckInDate(const std::string& checkInDate) {
    this->checkInDate = checkInDate;
}

std::string Booking::getCheckOutDate() const {
    return checkOutDate;
}
void Booking::setCheckOutDate(const std::string& checkOutDate) {
    this->checkOutDate = checkOutDate;
}

bool Booking::isCancelled() const {
    return cancelled;
}

void Booking::setCancelled(bool cancelled) {
    this->cancelled = cancelled;
}

bool Booking::isDeleted() const {
    return deleted;
}

void Booking::setDeleted(bool deleted) {
    this->deleted = deleted;
}

bool Booking::isValid() const {
    auto lockedCustomer = customer.lock();
    auto lockedRoom = room.lock();

    return lockedCustomer != nullptr &&
           lockedRoom != nullptr &&
           isIsoDateString(checkInDate) &&
           isIsoDateString(checkOutDate) &&
           checkOutDate > checkInDate;
}