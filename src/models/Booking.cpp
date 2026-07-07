#include "Booking.h"
#include "Customer.h"
#include "Room.h"
#include <string>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <QSqlQuery>
#include <QVariant>
#include <QSqlError>
#include <QDebug>
#include <QDate>

//The booking counter started at 1000 to be used as BookingID
int Booking::bookingCounter = 1000;

Booking::Booking() {
    bookingCounter++;
    this->bookingId = "BK" + std::to_string(bookingCounter);

    // Added: Initialize the dynamic lifecycle status to Upcoming by default (No Invoice exists yet)
    this->status = BookingStatus::Upcoming;

#pragma warning(suppress : 4996)
    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm* t = std::localtime(&now);

    std::stringstream ssIn;
    ssIn << std::put_time(t, "%Y-%m-%d");
    this->checkInDate = ssIn.str();

    this->checkOutDate = "";
}

// Read SQLite to get the current largest code to accurately restore the private static counter variable
void Booking::initCounterFromDatabase() {
    QSqlQuery query;
    if (query.exec("SELECT bookingId FROM Booking")) {
        int maxValue = bookingCounter;
        while (query.next()) {
            const QString id = query.value(0).toString();
            if (!id.startsWith("BK")) {
                continue;
            }
            bool ok = false;
            int numeric = id.mid(2).toInt(&ok);
            if (ok && numeric > maxValue) {
                maxValue = numeric;
            }
        }
        bookingCounter = maxValue;
    } else {
        qDebug() << "Error querying to reset the Booking counter:" << query.lastError().text();
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

// Added: Retreive the structural workflow state of this booking
BookingStatus Booking::getStatus() const {
    return status;
}

// Added: Transition the booking state (Shifting to Completed will trigger Invoice generation)
void Booking::setStatus(BookingStatus status) {
    this->status = status;
}

// Added: Standardized string serialization for UI text formatting and database storage
std::string Booking::getStatusString() const {
    switch (status) {
    case BookingStatus::Upcoming:  return "Upcoming";
    case BookingStatus::Active:    return "Active";
    case BookingStatus::Completed: return "Completed";
    case BookingStatus::Canceled:  return "Canceled";
    default:                       return "Unknown";
    }
}

bool Booking::isValid() const {
    auto lockedCustomer = customer.lock();
    auto lockedRoom = room.lock();

    return lockedCustomer != nullptr &&
           lockedRoom != nullptr &&
           !checkInDate.empty() &&
           !checkOutDate.empty() &&
           checkOutDate > checkInDate;
}