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

//The booking counter started at 1000 to be used as BookingID
int Booking::bookingCounter = 1000;

Booking::Booking() {
    bookingCounter++;
    this->bookingId = "BK" + std::to_string(bookingCounter);
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
    if (query.exec("SELECT MAX(bookingId) FROM Booking")) {
        if (query.next() && !query.value(0).isNull()) {
            std::string maxId = query.value(0).toString().toStdString();
            if (maxId.length() > 2 && maxId.substr(0, 2) == "BK") {
                std::string numStr = maxId.substr(2);
                bookingCounter = std::stoi(numStr);
            }
        }
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

bool Booking::isValid() const {
    auto lockedCustomer = customer.lock();
    auto lockedRoom = room.lock();

    return lockedCustomer != nullptr &&
           lockedRoom != nullptr &&
           !checkInDate.empty() &&
           !checkOutDate.empty() &&
           checkOutDate > checkInDate;
}