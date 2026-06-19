#include "HotelManager.h"

#include "DeluxeRoom.h"
#include "StandardRoom.h"
#include "SuiteRoom.h"

HotelManager::HotelManager() = default;

std::shared_ptr<Room> HotelManager::createRoom(RoomKind kind, int roomNumber, double baseRate) {
    switch (kind) {
    case RoomKind::Standard:
        return std::make_shared<StandardRoom>(roomNumber, baseRate > 0.0 ? baseRate : 100.0);
    case RoomKind::Deluxe:
        return std::make_shared<DeluxeRoom>(roomNumber, baseRate > 0.0 ? baseRate : 200.0);
    case RoomKind::Suite:
        return std::make_shared<SuiteRoom>(roomNumber, baseRate > 0.0 ? baseRate : 350.0);
    }

    return nullptr;
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
    for (const auto& room : rooms) {
        if (room && room->getRoomNumber() == roomNumber) {
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
        if (room && room->isAvailable()) {
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

    addRoom(createRoom(kind, roomNumber, baseRate));
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

    addCustomer(std::make_shared<Customer>(id, name, phone));
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

    if (checkInDate.empty() || checkOutDate.empty()) {
        errorMessage = "Check-in and check-out dates are required.";
        return false;
    }

    if (checkOutDate <= checkInDate) {
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

    if (!room->isAvailable()) {
        errorMessage = "Room is not available.";
        return false;
    }

    room->setAvailable(false);
    addBooking(std::make_shared<Booking>(bookingId, customer, room, checkInDate, checkOutDate));
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
            if (booking && booking->getRoom() && booking->getRoom()->getRoomNumber() == roomNumber) {
                errorMessage = "Cannot mark room available while it has an active booking.";
                return false;
            }
        }
    }

    room->setAvailable(available);
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

    return std::make_shared<Invoice>(invoiceId, booking, taxRate);
}
