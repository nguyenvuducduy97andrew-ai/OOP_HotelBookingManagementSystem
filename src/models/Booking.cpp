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
    this->checkedIn = false;
    this->checkedOut = false;
    this->checkOutDate = "";
    this->checkInDate = "";
    this->actualCheckInDate = "";
    this->actualCheckOutDate = "";
    this->createdAt = "";
    this->updatedAt = "";
    this->quotedUnitPrice = 0.0;
    this->quotedTaxRate = 0.10;
    this->adultCount = 1;
    this->childCount = 0;
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

bool Booking::isCheckedOut() const {
    return checkedOut;
}

void Booking::setCheckedOut(bool checkedOut) {
    this->checkedOut = checkedOut;
}

bool Booking::isCheckedIn() const { return checkedIn; }
void Booking::setCheckedIn(bool value) { checkedIn = value; }

std::string Booking::getActualCheckInDate() const { return actualCheckInDate; }
void Booking::setActualCheckInDate(const std::string& value) { actualCheckInDate = value; }
std::string Booking::getActualCheckOutDate() const { return actualCheckOutDate; }
void Booking::setActualCheckOutDate(const std::string& value) { actualCheckOutDate = value; }

std::string Booking::getEffectiveCheckOutDate() const {
    return actualCheckOutDate.empty() ? checkOutDate : actualCheckOutDate;
}

double Booking::getQuotedUnitPrice() const { return quotedUnitPrice; }
void Booking::setQuotedUnitPrice(double value) { quotedUnitPrice = value; }
double Booking::getQuotedTaxRate() const { return quotedTaxRate; }
void Booking::setQuotedTaxRate(double value) { quotedTaxRate = value; }

int Booking::getAdultCount() const { return adultCount; }
void Booking::setAdultCount(int value) { adultCount = value; }
int Booking::getChildCount() const { return childCount; }
void Booking::setChildCount(int value) { childCount = value; }

std::string Booking::getCancellationReason() const { return cancellationReason; }
void Booking::setCancellationReason(const std::string& value) { cancellationReason = value; }
std::string Booking::getCancelledAt() const { return cancelledAt; }
void Booking::setCancelledAt(const std::string& value) { cancelledAt = value; }
std::string Booking::getCreatedAt() const { return createdAt; }
void Booking::setCreatedAt(const std::string& value) { createdAt = value; }
std::string Booking::getUpdatedAt() const { return updatedAt; }
void Booking::setUpdatedAt(const std::string& value) { updatedAt = value; }

bool Booking::isValid() const {
    auto lockedCustomer = customer.lock();
    auto lockedRoom = room.lock();

    return lockedCustomer != nullptr &&
           lockedRoom != nullptr &&
           isIsoDateString(checkInDate) &&
           isIsoDateString(checkOutDate) &&
           checkOutDate >= checkInDate &&
           quotedUnitPrice > 0.0 && quotedTaxRate >= 0.0 && quotedTaxRate <= 1.0 &&
           adultCount > 0 && childCount >= 0;
}
