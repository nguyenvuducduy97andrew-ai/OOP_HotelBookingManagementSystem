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

// Modified: Completely skips Canceled bookings to avoid conflict, freeing up room availability
bool HotelManager::isRoomFreeForDates(
    const std::string& roomNumber,
    const std::string& checkIn,
    const std::string& checkOut,
    std::string& errorMessage) const
{
    for (const auto& booking : bookings) {
        if (!booking) continue;

        // CRITICAL BUG FIX: Disregard soft-deleted or canceled bookings entirely
        if (booking->getStatus() == BookingStatus::Canceled) {
            continue;
        }

        auto bookedRoom = booking->getRoom();
        if (!bookedRoom || bookedRoom->getRoomNumber() != roomNumber) continue;

        bool overlaps = checkIn < booking->getCheckOutDate() &&
                        booking->getCheckInDate() < checkOut;
        if (overlaps) {
            errorMessage = "Room " + roomNumber + " is already booked from "
                           + booking->getCheckInDate() + " to "
                           + booking->getCheckOutDate() + " (" + booking->getStatusString() + ").";
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

    return true;
}

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

std::vector<std::shared_ptr<Booking>> HotelManager::getBookingsForCustomer(const std::string &customerId) const
{
    std::vector<std::shared_ptr<Booking>> customerBookings;
    for (const auto &booking : bookings)
    {
        if (!booking) continue;

        const auto customer = booking->getCustomer();
        if (customer && customer->getCustomerId() == customerId)
        {
            customerBookings.push_back(booking);
        }
    }
    return customerBookings;
}

std::vector<std::shared_ptr<Booking>> HotelManager::getTodayCheckIns() const
{
    const std::string todayStr = QDate::currentDate().toString(Qt::ISODate).toStdString();
    return getArrivalsByDate(todayStr);
}

std::vector<std::shared_ptr<Booking>> HotelManager::getTodayCheckOuts() const
{
    const std::string todayStr = QDate::currentDate().toString(Qt::ISODate).toStdString();
    return getDeparturesByDate(todayStr);
}

// Added: Query mappings to filter structural bookings data into specialized layout sub-tabs
std::vector<std::shared_ptr<Booking>> HotelManager::getBookingsByStatus(BookingStatus status) const
{
    std::vector<std::shared_ptr<Booking>> filteredBookings;
    for (const auto &booking : bookings)
    {
        if (booking && booking->getStatus() == status)
        {
            filteredBookings.push_back(booking);
        }
    }
    return filteredBookings;
}

// Added: Extracted query helper targeting dynamic dashboard timelines instead of locking on hardcoded sysclock
std::vector<std::shared_ptr<Booking>> HotelManager::getArrivalsByDate(const std::string &dateStr) const
{
    std::vector<std::shared_ptr<Booking>> checkIns;
    for (const auto &booking : bookings)
    {
        if (booking && booking->getCheckInDate() == dateStr && booking->getStatus() != BookingStatus::Canceled)
        {
            checkIns.push_back(booking);
        }
    }
    return checkIns;
}

// Added: Extracted query helper targeting dashboard timeline tracking for departures
std::vector<std::shared_ptr<Booking>> HotelManager::getDeparturesByDate(const std::string &dateStr) const
{
    std::vector<std::shared_ptr<Booking>> checkOuts;
    for (const auto &booking : bookings)
    {
        if (booking && booking->getCheckOutDate() == dateStr && booking->getStatus() != BookingStatus::Canceled)
        {
            checkOuts.push_back(booking);
        }
    }
    return checkOuts;
}

// Modified: Disregards Canceled or Completed records when calculating active occupancy data
std::vector<std::shared_ptr<Room>> HotelManager::getRoomsByOccupancy(bool occupied) const
{
    std::vector<std::shared_ptr<Room>> matchingRooms;
    const QDate today = QDate::currentDate();

    for (const auto &room : rooms)
    {
        if (!room) continue;

        bool isOccupied = false;
        for (const auto &booking : bookings)
        {
            if (!booking || !booking->getRoom()) continue;

            // Skip checking since canceled/completed bookings means the client is not in the room
            if (booking->getStatus() == BookingStatus::Canceled || booking->getStatus() == BookingStatus::Completed) {
                continue;
            }

            if (booking->getRoom()->getRoomNumber() != room->getRoomNumber()) continue;

            const QDate checkIn = QDate::fromString(QString::fromStdString(booking->getCheckInDate()), Qt::ISODate);
            const QDate checkOut = QDate::fromString(QString::fromStdString(booking->getCheckOutDate()), Qt::ISODate);

            if (checkIn.isValid() && checkOut.isValid() && checkIn <= today && today < checkOut)
            {
                isOccupied = true;
                break;
            }
        }

        if (isOccupied == occupied)
        {
            matchingRooms.push_back(room);
        }
    }
    return matchingRooms;
}

// =========================
// ID generation
// =========================

std::string HotelManager::nextInvoiceId() const
{
    return "INV" + std::to_string(invoices.size() + 1001);
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
    if (!room)                 { errorMessage = "Room not found.";      return false; }

    auto customer = findCustomerById(customerId);
    if (!customer)            { errorMessage = "Customer not found.";  return false; }

    if (!isRoomFreeForDates(roomNumber, checkIn, checkOut, errorMessage)) return false;

    // Commit — only reached if everything passed
    auto booking = std::make_shared<Booking>();
    booking->setCustomer(customer);
    booking->setRoom(room);
    booking->setCheckInDate(checkIn);
    booking->setCheckOutDate(checkOut);

    // Explicitly set the core instantiation step to Upcoming state
    booking->setStatus(BookingStatus::Upcoming);

    // If checkIn date matches today, dynamically shift to Active immediately
    if (checkIn == QDate::currentDate().toString(Qt::ISODate).toStdString()) {
        booking->setStatus(BookingStatus::Active);
        room->setIsAvailable(false);
    }

    addBooking(booking);
    return true;
}

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
    invoice->setNights(nights);
    invoice->setPaymentDate(paymentDate);

    addInvoice(invoice);

    // Synchronously update the lifecycle state of the referenced booking record to Completed
    booking->setStatus(BookingStatus::Completed);
    if (booking->getRoom()) {
        booking->getRoom()->setIsAvailable(true); // Release the room instantly upon complete settlement
    }

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
            if (booking && booking->getStatus() == BookingStatus::Active &&
                booking->getRoom() && booking->getRoom()->getRoomNumber() == roomNumber)
            {
                errorMessage = "Cannot mark room available while it has an active booking.";
                return false;
            }
        }
    }

    room->setIsAvailable(available);
    return true;
}

// Added: Soft-cancels an online/upcoming booking safely without dropping db entries
bool HotelManager::cancelBooking(const std::string &bookingId, std::string &errorMessage)
{
    auto booking = findBookingById(bookingId);
    if (!booking) {
        errorMessage = "Booking record not found.";
        return false;
    }

    if (booking->getStatus() != BookingStatus::Upcoming) {
        errorMessage = "Only Upcoming reservations can be canceled.";
        return false;
    }

    // Process state shift to Canceled
    booking->setStatus(BookingStatus::Canceled);

    // Release the link back into the pool safely
    if (booking->getRoom()) {
        booking->getRoom()->setIsAvailable(true);
    }

    return true;
}

// =========================
// Delete methods (Hard deletions)
// =========================
bool HotelManager::deleteRoom(const std::string &roomNumber, std::string &errorMessage)
{
    auto room = findRoomByNumber(roomNumber);
    if (!room)
    {
        errorMessage = "Room not found.";
        return false;
    }

    for (const auto &booking : bookings)
    {
        if (booking && booking->getStatus() == BookingStatus::Active &&
            booking->getRoom() && booking->getRoom()->getRoomNumber() == roomNumber)
        {
            errorMessage = "Cannot delete room with active occupancy.";
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

    for (const auto &booking : bookings)
    {
        if (booking && booking->getStatus() == BookingStatus::Active &&
            booking->getCustomer() && booking->getCustomer()->getCustomerId() == customerId)
        {
            errorMessage = "Cannot delete customer with an active reservation stay.";
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