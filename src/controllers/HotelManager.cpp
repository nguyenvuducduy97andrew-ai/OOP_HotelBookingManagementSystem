#include "HotelManager.h"
#include <QDate>
#include <QString>
#include <algorithm>
#include <cctype>
#include <utility>

HotelManager::HotelManager() = default;

// =========================
// Internal add methods
// =========================

void HotelManager::addRoom(std::shared_ptr<Room> room)
{
    rooms.push_back(std::move(room));
}

void HotelManager::addCustomer(std::shared_ptr<Customer> customer)
{
    customers.push_back(std::move(customer));
}

void HotelManager::addBooking(std::shared_ptr<Booking> booking)
{
    bookings.push_back(std::move(booking));
}

void HotelManager::addInvoice(std::shared_ptr<Invoice> invoice)
{
    invoices.push_back(std::move(invoice));
}

// =========================
// Getters
// =========================

const std::vector<std::shared_ptr<Room>> &HotelManager::getRooms() const
{
    return rooms;
}

const std::vector<std::shared_ptr<Customer>> &HotelManager::getCustomers() const
{
    return customers;
}

const std::vector<std::shared_ptr<Booking>> &HotelManager::getBookings() const
{
    return bookings;
}

const std::vector<std::shared_ptr<Invoice>> &HotelManager::getInvoices() const
{
    return invoices;
}
// =========================
// Validation helpers
// =========================

bool HotelManager::isValidRoomNumber(const std::string &roomNumber) const
{
    return !roomNumber.empty() &&
           std::all_of(roomNumber.begin(), roomNumber.end(), [](unsigned char c)
                       { return std::isalnum(c); });
}

bool HotelManager::validateRoomInput(
    const std::string &roomNumber,
    double baseRate,
    std::string &errorMessage) const
{
    if (!isValidRoomNumber(roomNumber))
    {
        errorMessage = "Room number must not be empty and must contain only letters and numbers.";
        return false;
    }

    if (baseRate <= 0)
    {
        errorMessage = "Base rate must be greater than zero.";
        return false;
    }

    if (roomNumberExists(roomNumber))
    {
        errorMessage = "Room number already exists.";
        return false;
    }

    return true;
}

bool HotelManager::validateCustomerInput(
    const std::string &id,
    const std::string &name,
    const std::string &phone,
    std::string &errorMessage) const
{
    if (id.empty())
    {
        errorMessage = "Customer ID is required.";
        return false;
    }

    if (name.empty())
    {
        errorMessage = "Customer name is required.";
        return false;
    }

    if (phone.empty())
    {
        errorMessage = "Phone number is required.";
        return false;
    }

    if (customerIdExists(id))
    {
        errorMessage = "Customer ID already exists.";
        return false;
    }

    return true;
}
bool HotelManager::validateBookingDates(
    const std::string& checkIn,
    const std::string& checkOut,
    std::string& errorMessage) const
{
    if (checkIn.empty() || checkOut.empty()) {
        errorMessage = "Dates cannot be empty.";
        return false;
    }
    if (checkOut <= checkIn) {
        errorMessage = "Check-out must be after check-in.";
        return false;
    }
    return true;
}

bool HotelManager::isRoomFreeForDates(
    const std::string& roomNumber,
    const std::string& checkIn,
    const std::string& checkOut,
    std::string& errorMessage) const
{
    for (const auto& booking : bookings) {
        auto bookedRoom = booking->getRoom();
        if (!bookedRoom || bookedRoom->getRoomNumber() != roomNumber) continue;

        bool overlaps = checkIn  < booking->getCheckOutDate() &&
                        booking->getCheckInDate() < checkOut;
        if (overlaps) {
            errorMessage = "Room " + roomNumber + " is already booked from "
                         + booking->getCheckInDate() + " to "
                         + booking->getCheckOutDate() + ".";
            return false;
        }
    }
    return true;
}
bool HotelManager::validateBookingInput(
    const std::string &customerId,
    const std::string &roomNumber,
    const std::string &checkInDate,
    const std::string &checkOutDate,
    std::string &errorMessage) const
{
    const auto checkIn = QDate::fromString(QString::fromStdString(checkInDate), Qt::ISODate);
    const auto checkOut = QDate::fromString(QString::fromStdString(checkOutDate), Qt::ISODate);

    if (!checkIn.isValid() || !checkOut.isValid())
    {
        errorMessage = "Check-in and check-out dates must use ISO format (YYYY-MM-DD).";
        return false;
    }

    if (checkOut <= checkIn)
    {
        errorMessage = "Check-out date must be after check-in date.";
        return false;
    }

    const auto customer = findCustomerById(customerId);
    if (!customer)
    {
        errorMessage = "Customer not found.";
        return false;
    }

    const auto room = findRoomByNumber(roomNumber);
    if (!room)
    {
        errorMessage = "Room not found.";
        return false;
    }

    if (!room->getIsAvailable())
    {
        errorMessage = "Room is not available.";
        return false;
    }

    return true;
}

// Modified: Updated signature to accept nights to validate stay duration bounds
bool HotelManager::validateInvoiceInput(
    const std::string &invoiceId,
    const std::string &bookingId,
    double taxRate,
    int nights,
    std::string &errorMessage) const
{
    if (invoiceId.empty())
    {
        errorMessage = "Invoice ID is required.";
        return false;
    }

    if (taxRate < 0)
    {
        errorMessage = "Tax rate must not be negative.";
        return false;
    }

    // Added: Validate that the calculated stay duration is at least 1 night
    if (nights <= 0)
    {
        errorMessage = "Stay duration in nights must be greater than zero.";
        return false;
    }

    const auto booking = findBookingById(bookingId);
    if (!booking)
    {
        errorMessage = "Booking not found.";
        return false;
    }

    return true;
}

// =========================
// Existence checks
// =========================

bool HotelManager::roomNumberExists(const std::string &roomNumber) const
{
    return findRoomByNumber(roomNumber) != nullptr;
}

bool HotelManager::customerIdExists(const std::string &customerId) const
{
    return findCustomerById(customerId) != nullptr;
}

bool HotelManager::bookingIdExists(const std::string &bookingId) const
{
    return findBookingById(bookingId) != nullptr;
}

bool HotelManager::invoiceIdExists(const std::string &invoiceId) const
{
    return findInvoiceById(invoiceId) != nullptr;
}
// =========================
// Find methods
// =========================

std::shared_ptr<Room> HotelManager::findRoomByNumber(const std::string &roomNumber) const
{
    for (const auto &room : rooms)
    {
        if (room && room->getRoomNumber() == roomNumber)
        {
            return room;
        }
    }

    return nullptr;
}

std::shared_ptr<Customer> HotelManager::findCustomerById(const std::string &customerId) const
{
    for (const auto &customer : customers)
    {
        if (customer && customer->getCustomerId() == customerId)
        {
            return customer;
        }
    }

    return nullptr;
}

std::shared_ptr<Booking> HotelManager::findBookingById(const std::string &bookingId) const
{
    for (const auto &booking : bookings)
    {
        if (booking && booking->getBookingId() == bookingId)
        {
            return booking;
        }
    }

    return nullptr;
}

std::shared_ptr<Invoice> HotelManager::findInvoiceById(const std::string &invoiceId) const
{
    for (const auto &invoice : invoices)
    {
        if (invoice && invoice->getInvoiceId() == invoiceId)
        {
            return invoice;
        }
    }

    return nullptr;
}
// =========================
// Queries
// =========================

std::vector<std::shared_ptr<Room>> HotelManager::getAvailableRooms() const
{
    std::vector<std::shared_ptr<Room>> availableRooms;

    for (const auto &room : rooms)
    {
        if (room && room->getIsAvailable())
        {
            availableRooms.push_back(room);
        }
    }

    return availableRooms;
}

// =========================
// ID generation
// =========================

std::string HotelManager::nextInvoiceId() const
{
    return "INV" + std::to_string(bookings.size() + 1);
}

// =========================
// Use-case methods
// =========================

bool HotelManager::registerRoom(
    RoomType kind,
    const std::string &roomNumber,
    double baseRate,
    std::string &errorMessage)
{
    if (!validateRoomInput(roomNumber, baseRate, errorMessage))
    {
        return false;
    }

    auto room = RoomFactory::createRoom(kind, roomNumber, baseRate);

    if (!room)
    {
        errorMessage = "Failed to create room.";
        return false;
    }

    addRoom(room);
    return true;
}

bool HotelManager::registerCustomer(
    const std::string &id,
    const std::string &name,
    const std::string &phone,
    std::string &errorMessage)
{
    if (!validateCustomerInput(id, name, phone, errorMessage))
    {
        return false;
    }

    auto customer = std::make_shared<Customer>();
    customer->setCustomerId(id);
    customer->setName(name);
    customer->setPhoneNumber(phone);

    addCustomer(customer);
    return true;
}

// Modified: Refactored entirely to assign dates directly as std::string instead of QDate
bool HotelManager::createBooking(
    const std::string& customerId,
    const std::string& roomNumber,
    const std::string& checkIn,
    const std::string& checkOut,
    std::string& errorMessage)
{
    if (!validateBookingInput(customerId, roomNumber, checkIn, checkOut, errorMessage)) return false;
    if (!validateBookingDates(checkIn, checkOut, errorMessage))   return false;

    auto room = findRoomByNumber(roomNumber);
    if (!room)                { errorMessage = "Room not found.";      return false; }
    if (!room->getIsAvailable()) { errorMessage = "Room unavailable.";    return false; }

    auto customer = findCustomerById(customerId);
    if (!customer)            { errorMessage = "Customer not found.";  return false; }

    if (!isRoomFreeForDates(roomNumber, checkIn, checkOut, errorMessage)) return false;

    // Commit — only reached if everything passed
    auto booking = std::make_shared<Booking>();
    booking->setCustomer(customer);  // Pass shared_ptr directly
    booking->setRoom(room);          // Pass shared_ptr directly

    // Modified: Directly passing string to core properties without wrapping in QDate
    booking->setCheckInDate(checkIn);
    booking->setCheckOutDate(checkOut);

    room->setIsAvailable(false);
    addBooking(booking);
    return true;
}

// Modified: Updated signature and body to inject UI-calculated 'nights' and 'paymentDate' down into the core invoice entity
bool HotelManager::createInvoice(
    const std::string &invoiceId,
    const std::string &bookingId,
    double taxRate,
    int nights,
    const std::string &paymentDate,
    std::string &errorMessage)
{
    if (!validateInvoiceInput(invoiceId, bookingId, taxRate, nights, errorMessage))
    {
        return false;
    }

    const auto booking = findBookingById(bookingId);
    if (!booking)
    {
        errorMessage = "Booking not found.";
        return false;
    }

    auto invoice = std::make_shared<Invoice>();
    invoice->setInvoiceId(invoiceId);
    invoice->setBooking(booking);
    invoice->setTaxRate(taxRate);
    invoice->setNights(nights);           // Added: Storing calculated night count from view layer
    invoice->setPaymentDate(paymentDate); // Added: Storing checkout billing timestamp string

    addInvoice(invoice);
    return true;
}

bool HotelManager::setRoomAvailability(
    const std::string &roomNumber,
    bool available,
    std::string &errorMessage)
{
    const auto room = findRoomByNumber(roomNumber);

    if (!room)
    {
        errorMessage = "Room not found.";
        return false;
    }

    if (available)
    {
        for (const auto &booking : bookings)
        {
            if (booking &&
                booking->getRoom() &&
                booking->getRoom()->getRoomNumber() == roomNumber)
            {
                errorMessage = "Cannot mark room available while it has an active booking.";
                return false;
            }
        }
    }

    room->setIsAvailable(available);
    return true;
}


// =========================
// Delete methods
// =========================
bool HotelManager::deleteRoom(const std::string &roomNumber, std::string &errorMessage)
{
    auto room = findRoomByNumber(roomNumber);
    if (!room)
    {
        errorMessage = "Room not found.";
        return false;
    }

    // Check if the room is associated with any active bookings
    for (const auto &booking : bookings)
    {
        if (booking && booking->getRoom() && booking->getRoom()->getRoomNumber() == roomNumber)
        {
            errorMessage = "Cannot delete room with active bookings.";
            return false;
        }
    }

    rooms.erase(std::remove(rooms.begin(), rooms.end(), room), rooms.end());
    return true;
}
bool HotelManager::deleteCustomer(const std::string &customerId, std::string &errorMessage)
{
    auto customer = findCustomerById(customerId);
    if (!customer)
    {
        errorMessage = "Customer not found.";
        return false;
    }

    // Check if the customer is associated with any active bookings
    for (const auto &booking : bookings)
    {
        if (booking && booking->getCustomer() && booking->getCustomer()->getCustomerId() == customerId)
        {
            errorMessage = "Cannot delete customer with active bookings.";
            return false;
        }
    }

    customers.erase(std::remove(customers.begin(), customers.end(), customer), customers.end());
    return true;
}
bool HotelManager::deleteBooking(const std::string &bookingId, std::string &errorMessage)
{
    auto booking = findBookingById(bookingId);
    if (!booking)
    {
        errorMessage = "Booking not found.";
        return false;
    }


    bookings.erase(std::remove(bookings.begin(), bookings.end(), booking), bookings.end());
    return true;
}
bool HotelManager::deleteInvoice(const std::string &invoiceId, std::string &errorMessage)
{
    auto invoice = findInvoiceById(invoiceId);
    if (!invoice)
    {
        errorMessage = "Invoice not found.";
        return false;
    }

    invoices.erase(std::remove(invoices.begin(), invoices.end(), invoice), invoices.end());
    return true;
}