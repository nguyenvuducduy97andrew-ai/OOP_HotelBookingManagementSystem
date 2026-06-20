#include "HotelManager.h"
#include "DeluxeRoom.h"
#include "StandardRoom.h"
#include "SuiteRoom.h"
#include <QDate>
#include <QString>

namespace {

RoomType toRoomType(RoomKind kind) {
    switch (kind) {
    case RoomKind::Standard:
        return RoomType::Standard;
    case RoomKind::Deluxe:
        return RoomType::Deluxe;
    case RoomKind::Suite:
        return RoomType::Suite;
    }

    return RoomType::Standard;
}

} // namespace

HotelManager::HotelManager() = default;

std::shared_ptr<Room> HotelManager::createRoom(RoomKind kind, int roomNumber, double baseRate) {
    RoomFactory factory;
    const auto room = factory.createRoom(
        toRoomType(kind),
        std::to_string(roomNumber),
        baseRate > 0.0 ? baseRate : 0.0);

    return std::shared_ptr<Room>(room);
}

void HotelManager::addRoom(std::shared_ptr<Room> room) {
    rooms.push_back(std::move(room));
}

const std::vector<std::shared_ptr<Room>>& HotelManager::getRooms() const {
    return rooms;
}

void HotelManager::addCustomer(std::shared_ptr<Customer> customer) {
    customers.push_back(std::move(customer));
}

const std::vector<std::shared_ptr<Customer>>& HotelManager::getCustomers() const {
    return customers;
}

void HotelManager::addBooking(std::shared_ptr<Booking> booking) {
    bookings.push_back(std::move(booking));
}

const std::vector<std::shared_ptr<Booking>>& HotelManager::getBookings() const {
    return bookings;
}

bool HotelManager::roomNumberExists(int roomNumber) const {
    return findRoomByNumber(roomNumber) != nullptr;
}

bool HotelManager::customerIdExists(const std::string& customerId) const {
    return findCustomerById(customerId) != nullptr;
}

bool HotelManager::bookingIdExists(const std::string& bookingId) const {
    return findBookingById(bookingId) != nullptr;
}

std::shared_ptr<Room> HotelManager::findRoomByNumber(int roomNumber) const {
    const std::string targetRoomNumber = std::to_string(roomNumber);
    for (const auto& room : rooms) {
        if (room && room->getRoomNumber() == targetRoomNumber) {
            return room;
        }
    }
    return nullptr;
}

std::shared_ptr<Customer> HotelManager::findCustomerById(const std::string& customerId) const {
    for (const auto& customer : customers) {
        if (customer && customer->getCustomerId() == customerId) {
            return customer;
        }
    }
    return nullptr;
}

std::shared_ptr<Booking> HotelManager::findBookingById(const std::string& bookingId) const {
    for (const auto& booking : bookings) {
        if (booking && booking->getBookingId() == bookingId) {
            return booking;
        }
    }
    return nullptr;
}

std::vector<std::shared_ptr<Room>> HotelManager::getAvailableRooms() const {
    std::vector<std::shared_ptr<Room>> availableRooms;
    for (const auto& room : rooms) {
        if (room && room->getIsAvailable()) {
            availableRooms.push_back(room);
        }
    }
    return availableRooms;
}

std::string HotelManager::nextBookingId() const {
    return "BK" + std::to_string(bookings.size() + 1);
}

std::string HotelManager::nextInvoiceId() const {
    return "INV" + std::to_string(bookings.size() + 1);
}

bool HotelManager::addRoomIfValid(RoomKind kind, int roomNumber, double baseRate, std::string& errorMessage) {
    if (roomNumber <= 0) {
        errorMessage = "Room number must be greater than zero.";
        return false;
    }

    if (roomNumberExists(roomNumber)) {
        errorMessage = "Room number already exists.";
        return false;
    }

    const auto room = createRoom(kind, roomNumber, baseRate);
    if (!room) {
        errorMessage = "Failed to create room.";
        return false;
    }

    addRoom(room);
    return true;
}

bool HotelManager::addCustomerIfValid(const std::string& id, const std::string& name, const std::string& phone,
                                      std::string& errorMessage) {
    if (id.empty()) {
        errorMessage = "Customer ID is required.";
        return false;
    }

    if (name.empty()) {
        errorMessage = "Customer name is required.";
        return false;
    }

    if (customerIdExists(id)) {
        errorMessage = "Customer ID already exists.";
        return false;
    }

    auto customer = std::make_shared<Customer>();
    customer->setCustomerId(id);
    customer->setName(name);
    customer->setPhoneNumber(phone);
    addCustomer(customer);
    return true;
}

bool HotelManager::addBookingIfValid(const std::string& bookingId, const std::string& customerId, int roomNumber,
                                     const std::string& checkInDate, const std::string& checkOutDate,
                                     std::string& errorMessage) {
    if (bookingId.empty()) {
        errorMessage = "Booking ID is required.";
        return false;
    }

    if (bookingIdExists(bookingId)) {
        errorMessage = "Booking ID already exists.";
        return false;
    }

    const auto checkIn = QDate::fromString(QString::fromStdString(checkInDate), Qt::ISODate);
    const auto checkOut = QDate::fromString(QString::fromStdString(checkOutDate), Qt::ISODate);
    if (!checkIn.isValid() || !checkOut.isValid()) {
        errorMessage = "Check-in and check-out dates must use ISO format (YYYY-MM-DD).";
        return false;
    }

    if (checkOut <= checkIn) {
        errorMessage = "Check-out date must be after check-in date.";
        return false;
    }

    const auto customer = findCustomerById(customerId);
    if (!customer) {
        errorMessage = "Customer not found.";
        return false;
    }

    const auto room = findRoomByNumber(roomNumber);
    if (!room) {
        errorMessage = "Room not found.";
        return false;
    }

    if (!room->getIsAvailable()) {
        errorMessage = "Room is not available.";
        return false;
    }

    auto booking = std::make_shared<Booking>();
    booking->setBookingId(bookingId);
    booking->setCustomer(customer.get());
    booking->setRoom(room.get());
    booking->setCheckInDate(checkIn);
    booking->setCheckOutDate(checkOut);

    room->setIsAvailable(false);
    addBooking(booking);
    return true;
}

bool HotelManager::setRoomAvailability(int roomNumber, bool available, std::string& errorMessage) {
    const auto room = findRoomByNumber(roomNumber);
    if (!room) {
        errorMessage = "Room not found.";
        return false;
    }

    if (available) {
        for (const auto& booking : bookings) {
            if (booking && booking->getRoom() && booking->getRoom()->getRoomNumber() == std::to_string(roomNumber)) {
                errorMessage = "Cannot mark room available while it has an active booking.";
                return false;
            }
        }
    }

    room->setIsAvailable(available);
    return true;
}

std::shared_ptr<Invoice> HotelManager::createInvoice(const std::string& invoiceId, const std::string& bookingId,
                                                     int days, double taxRate, std::string& errorMessage) const {
    if (invoiceId.empty()) {
        errorMessage = "Invoice ID is required.";
        return nullptr;
    }

    if (days <= 0) {
        errorMessage = "Number of days must be greater than zero.";
        return nullptr;
    }

    const auto booking = findBookingById(bookingId);
    if (!booking) {
        errorMessage = "Booking not found.";
        return nullptr;
    }

    const auto room = booking->getRoom();
    if (!room) {
        errorMessage = "Booking has no room assigned.";
        return nullptr;
    }

    auto invoice = std::make_shared<Invoice>();
    invoice->setInvoiceId(invoiceId);
    invoice->setBooking(booking.get());
    const double subtotal = room->getBasePrice() * static_cast<double>(days);
    invoice->setTotalAmount(subtotal * (1.0 + taxRate));
    invoice->setPaymentDate(QDate::currentDate());
    return invoice;
}
