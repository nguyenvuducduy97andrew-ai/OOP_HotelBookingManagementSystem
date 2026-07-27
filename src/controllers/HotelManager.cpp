#include "HotelManager.h"
#include "BookingService.h"
#include "InvoiceService.h"
#include "ReservationService.h"
#include <QDate>
#include <QList>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <algorithm>
#include <cctype>
#include <utility>

namespace {
std::string collapseWhitespace(const std::string& value)
{
    return QString::fromStdString(value).simplified().toStdString();
}

bool hasMixedCaseLetters(const QString& value)
{
    bool hasUpper = false;
    bool hasLower = false;

    for (const QChar ch : value) {
        if (ch.isUpper()) {
            hasUpper = true;
        } else if (ch.isLower()) {
            hasLower = true;
        }

        if (hasUpper && hasLower) {
            return true;
        }
    }

    return false;
}

bool isSingleNameTokenValid(const QString& token)
{
    static const QRegularExpression tokenPattern(QStringLiteral(R"(^[\p{L}][\p{L}'’\-]*$)"));
    return tokenPattern.match(token).hasMatch();
}
}

std::string bookingStateToString(BookingState state)
{
    switch (state)
    {
    case BookingState::UPCOMING:
        return "Upcoming";
    case BookingState::ACTIVE:
        return "Active";
    case BookingState::COMPLETED:
        return "Completed";
    case BookingState::CANCELLED:
        return "Cancelled";
    default:
        return "Unknown";
    }
}

HotelManager::HotelManager() = default;

bool HotelManager::isValidCustomerIdFormat(const std::string& customerId)
{
    // Modified and optimized performance: validate the supported national ID formats before data reaches persistence.
    const QString id = QString::fromStdString(customerId).trimmed().toUpper();
    static const QList<QRegularExpression> idPatterns = {
        QRegularExpression(QStringLiteral(R"(^\d{12}$)")),
        QRegularExpression(QStringLiteral(R"(^\d{9}$)")),
        QRegularExpression(QStringLiteral(R"(^[A-CEGHJ-PR-TW-Z]{2}\d{6}[A-D]$)")),
        QRegularExpression(QStringLiteral(R"(^[STFGM]\d{7}[A-Z]$)")),
        QRegularExpression(QStringLiteral(R"(^\d{13}$)")),
        QRegularExpression(QStringLiteral(R"(^\d{10}$)")),
        QRegularExpression(QStringLiteral(R"(^[A-Z0-9]{9}$)"))
    };

    return std::any_of(idPatterns.cbegin(), idPatterns.cend(), [&id](const QRegularExpression& pattern) {
        return pattern.match(id).hasMatch();
    });
}

bool HotelManager::isValidCustomerNameFormat(const std::string& customerName)
{
    const QString normalized = QString::fromStdString(collapseWhitespace(customerName));
    if (normalized.isEmpty()) {
        return false;
    }

    const QStringList tokens = normalized.split(' ', Qt::SkipEmptyParts);
    if (tokens.size() < 2) {
        return false;
    }

    for (const QString& token : tokens) {
        if (!isSingleNameTokenValid(token)) {
            return false;
        }
    }

    return hasMixedCaseLetters(normalized);
}

bool HotelManager::isValidPhoneNumberFormat(const std::string& phoneNumber)
{
    // Modified and optimized performance: validate E.164-style numbers for the selectable country dialing codes.
    const QString phone = QString::fromStdString(collapseWhitespace(phoneNumber));
    static const QRegularExpression phonePattern(
        QStringLiteral(R"(^(?:\+84\d{9}|\+1\d{10}|\+60\d{9}|\+44\d{10}|\+81\d{10}|\+65\d{8}|\+82\d{10}|\+66\d{9}|\+61\d{9}|\+49\d{10})$)"));
    return phonePattern.match(phone).hasMatch();
}

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

void HotelManager::clearAll()
{
    rooms.clear();
    customers.clear();
    bookings.clear();
    invoices.clear();
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
    if (!isValidCustomerIdFormat(id))
    {
        errorMessage = "Customer ID does not match a supported national ID format.";
        return false;
    }

    if (!isValidCustomerNameFormat(name))
    {
        errorMessage = "Customer name must have at least 2 words and include both uppercase and lowercase letters.";
        return false;
    }

    if (!isValidPhoneNumberFormat(phone))
    {
        errorMessage = "Phone number does not match the selected country format.";
        return false;
    }

    if (customerIdExists(id))
    {
        errorMessage = "Customer ID already exists.";
        return false;
    }

    return true;
}

bool HotelManager::isValidDateString(
    const std::string &dateString,
    std::string &errorMessage) const
{
    if (dateString.empty())
    {
        errorMessage = "Date cannot be empty.";
        return false;
    }

    const QDate date = QDate::fromString(QString::fromStdString(dateString), Qt::ISODate);
    if (!date.isValid())
    {
        errorMessage = "Date must use ISO format (YYYY-MM-DD).";
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

std::shared_ptr<Invoice> HotelManager::findInvoiceForBooking(const std::string &bookingId) const
{
    for (const auto &invoice : invoices)
    {
        if (invoice && invoice->getBooking() && invoice->getBooking()->getBookingId() == bookingId)
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
        if (!room || !room->getIsAvailable() || room->isArchived())
        {
            continue;
        }

        bool isOccupied = false;
        for (const auto &booking : bookings)
        {
            if (!booking || !booking->getRoom())
            {
                continue;
            }

            if (booking->isDeleted())
            {
                continue;
            }

            if (booking->getRoom()->getRoomNumber() != room->getRoomNumber())
            {
                continue;
            }

            if (getBookingState(*booking) == BookingState::ACTIVE)
            {
                isOccupied = true;
                break;
            }
        }

        if (!isOccupied)
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
        if (!booking)
            continue;

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
std::vector<std::shared_ptr<Booking>> HotelManager::getBookingsByStatus(BookingState state) const
{
    std::vector<std::shared_ptr<Booking>> filteredBookings;
    for (const auto &booking : bookings)
    {
        if (booking && !booking->isDeleted() && getBookingState(*booking) == state)
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
        if (booking && booking->getCheckInDate() == dateStr && !booking->isCancelled() && !booking->isDeleted())
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
        if (booking && booking->getCheckOutDate() == dateStr && !booking->isCancelled() && !booking->isDeleted())
        {
            checkOuts.push_back(booking);
        }
    }
    return checkOuts;
}

BookingState HotelManager::getBookingState(const Booking &booking) const
{
    if (booking.isCancelled())
    {
        return BookingState::CANCELLED;
    }

    // Modified and optimized performance: make completion depend on the persisted checkout action instead of the planned departure date.
    if (booking.isCheckedOut())
    {
        return BookingState::COMPLETED;
    }

    const QDate today = QDate::currentDate();
    const QDate checkIn = QDate::fromString(QString::fromStdString(booking.getCheckInDate()), Qt::ISODate);
    const QDate checkOut = QDate::fromString(QString::fromStdString(booking.getCheckOutDate()), Qt::ISODate);

    if (!checkIn.isValid() || !checkOut.isValid())
    {
        return BookingState::UPCOMING;
    }

    if (today < checkIn)
    {
        return BookingState::UPCOMING;
    }

    return BookingState::ACTIVE;
}

// Modified: Disregards Canceled or Completed records when calculating active occupancy data
std::vector<std::shared_ptr<Room>> HotelManager::getRoomsByOccupancy(bool occupied) const
{
    std::vector<std::shared_ptr<Room>> matchingRooms;

    for (const auto &room : rooms)
    {
        if (!room)
            continue;

        bool isOccupied = false;
        for (const auto &booking : bookings)
        {
            if (!booking || booking->isDeleted() || !booking->getRoom())
                continue;

            const BookingState bookingState = getBookingState(*booking);
            if (bookingState != BookingState::ACTIVE)
                continue;

            if (booking->getRoom()->getRoomNumber() != room->getRoomNumber())
                continue;

            isOccupied = true;
            break;
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
    int maxId = 1000;
    for (const auto &invoice : invoices)
    {
        if (!invoice)
            continue;
        const std::string id = invoice->getInvoiceId();
        if (id.rfind("INV", 0) == 0 && id.size() > 3)
        {
            try
            {
                int numeric = std::stoi(id.substr(3));
                maxId = std::max(maxId, numeric);
            }
            catch (...)
            {
                continue;
            }
        }
    }
    return "INV" + std::to_string(maxId + 1);
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

bool HotelManager::resolveCustomerForBooking(
    const std::string &id,
    const std::string &name,
    const std::string &phone,
    std::string &errorMessage)
{
    auto customer = findCustomerById(id);
    if (!customer) {
        return registerCustomer(id, name, phone, errorMessage);
    }

    if (customer->isArchived()) {
        errorMessage = "Archived customers cannot be used for bookings.";
        return false;
    }

    const QString storedName = QString::fromStdString(customer->getName()).simplified();
    const QString inputName = QString::fromStdString(collapseWhitespace(name)).simplified();
    if (storedName.compare(inputName, Qt::CaseInsensitive) != 0) {
        errorMessage = "Customer name does not match the registered customer for this ID.";
        return false;
    }

    if (customer->getPhoneNumber() != phone) {
        errorMessage = "Phone number does not match the registered customer for this ID.";
        return false;
    }

    return true;
}

bool HotelManager::createBooking(
    const std::string &customerId,
    const std::string &roomNumber,
    const std::string &checkIn,
    const std::string &checkOut,
    std::string &errorMessage)
{
    // Modified and optimized performance: delegate booking creation to the focused service while preserving the existing UI-facing API.
    return ReservationService(*this).createBooking(customerId, roomNumber, checkIn, checkOut, errorMessage);
}

bool HotelManager::updateBooking(
    const std::string &bookingId,
    const std::string &customerId,
    const std::string &roomNumber,
    const std::string &checkInDate,
    const std::string &checkOutDate,
    std::string &errorMessage)
{
    return ReservationService(*this).updateBooking(
        bookingId, customerId, roomNumber, checkInDate, checkOutDate, errorMessage);
}

bool HotelManager::completeBooking(
    const std::string &bookingId,
    const std::string &actualCheckoutDate,
    std::string &errorMessage)
{
    return ReservationService(*this).completeBooking(bookingId, actualCheckoutDate, errorMessage);
}

bool HotelManager::createInvoice( // In practice, checkout first, then create invoice for completed booking
    const std::string &invoiceId,
    const std::string &bookingId,
    double taxRate,
    int nights, // Intentionally for partial stays
    const std::string &paymentDate,
    std::string &errorMessage)
{
    // Modified and optimized performance: keep invoice validation and creation isolated from the in-memory hotel store.
    return ReservationService(*this).createInvoice(invoiceId, bookingId, taxRate, nights, paymentDate, errorMessage);
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

    if (!available)
    {
        for (const auto &booking : bookings)
        {
            if (!booking || booking->isCancelled() || booking->isDeleted())
                continue;

            if (!booking->getRoom() || booking->getRoom()->getRoomNumber() != roomNumber)
                continue;

            if (getBookingState(*booking) == BookingState::ACTIVE)
            {
                errorMessage = "Cannot mark room unavailable while a guest is checked in";
                return false;
            }
        }
    }

    room->setIsAvailable(available);
    return true;
}

// Added: Archive a room or customer to hide them from active listings without deleting historical data
bool HotelManager::archiveRoom(const std::string &roomNumber, std::string &errorMessage)
{
    auto room = findRoomByNumber(roomNumber);
    if (!room)
    {
        errorMessage = "Room not found.";
        return false;
    }

    for (const auto &booking : bookings)
    {
        if (!booking || booking->isCancelled() || booking->isDeleted())
            continue;

        auto bookedRoom = booking->getRoom();
        if (!bookedRoom || bookedRoom->getRoomNumber() != roomNumber)
            continue;

        if (getBookingState(*booking) == BookingState::ACTIVE)
        {
            errorMessage = "Cannot archive room while a guest is checked in.";
            return false;
        }
    }

    room->setArchived(true);
    return true;
}

bool HotelManager::archiveCustomer(const std::string &customerId, std::string &errorMessage)
{
    auto customer = findCustomerById(customerId);
    if (!customer)
    {
        errorMessage = "Customer not found.";
        return false;
    }

    for (const auto &booking : bookings)
    {
        if (!booking || booking->isCancelled() || booking->isDeleted())
            continue;

        auto bookingCustomer = booking->getCustomer();
        if (!bookingCustomer || bookingCustomer->getCustomerId() != customerId)
            continue;

        if (getBookingState(*booking) == BookingState::ACTIVE)
        {
            errorMessage = "Cannot archive customer while they have an active booking.";
            return false;
        }
    }

    customer->setArchived(true);
    return true;
}

// Added: Unarchive a room or customer to restore them to active listings
bool HotelManager::restoreRoom(const std::string &roomNumber, std::string &errorMessage)
{
    auto room = findRoomByNumber(roomNumber);
    if (!room)
    {
        errorMessage = "Room not found.";
        return false;
    }

    room->setArchived(false);
    return true;
}

bool HotelManager::restoreCustomer(const std::string &customerId, std::string &errorMessage)
{
    auto customer = findCustomerById(customerId);
    if (!customer)
    {
        errorMessage = "Customer not found.";
        return false;
    }

    customer->setArchived(false);
    return true;
}

//Data Manager integration methods for persistence
bool HotelManager::restoreBookingFromDatabase(
    const std::string &bookingId,
    const std::string &customerId,
    const std::string &roomNumber,
    const std::string &checkInDate,
    const std::string &checkOutDate,
    bool cancelled,
    bool deleted,
    bool checkedOut,
    std::string &errorMessage)
{
    if (bookingId.empty())
    {
        errorMessage = "Persisted booking ID is empty.";
        return false;
    }

    if (bookingIdExists(bookingId))
    {
        errorMessage =
            "Duplicate persisted booking ID: " + bookingId;
        return false;
    }

    auto customer = findCustomerById(customerId);
    if (!customer)
    {
        errorMessage =
            "Booking references missing customer: " + customerId;
        return false;
    }

    auto room = findRoomByNumber(roomNumber);
    if (!room)
    {
        errorMessage =
            "Booking references missing room: " + roomNumber;
        return false;
    }

    if (!isValidDateString(checkInDate, errorMessage) || !isValidDateString(checkOutDate, errorMessage))
    {
        return false;
    }

    const QDate checkIn = QDate::fromString(QString::fromStdString(checkInDate), Qt::ISODate);
    const QDate checkOut = QDate::fromString(QString::fromStdString(checkOutDate), Qt::ISODate);
    // Modified and optimized performance: restore completed same-day stays from SQLite while rejecting only a checkout before check-in.
    if (checkOut < checkIn)
    {
        errorMessage = "Check-out cannot be before check-in.";
        return false;
    }

    auto booking = std::make_shared<Booking>();
    booking->setBookingId(bookingId);
    booking->setCustomer(customer);
    booking->setRoom(room);
    booking->setCheckInDate(checkInDate);
    booking->setCheckOutDate(checkOutDate);
    booking->setCancelled(cancelled);
    booking->setDeleted(deleted);
    booking->setCheckedOut(checkedOut);

    addBooking(booking);
    return true;
}

bool HotelManager::restoreCustomerFromDatabase(
    const std::string &customerId,
    const std::string &name,
    const std::string &phone,
    bool archived,
    std::string &errorMessage)
{
    if (customerId.empty())
    {
        errorMessage = "Persisted customer ID is empty.";
        return false;
    }

    if (customerIdExists(customerId))
    {
        errorMessage = "Duplicate persisted customer ID: " + customerId;
        return false;
    }

    if (name.empty() || phone.empty())
    {
        errorMessage = "Persisted customer record is incomplete.";
        return false;
    }

    auto customer = std::make_shared<Customer>();
    customer->setCustomerId(customerId);
    customer->setName(name);
    customer->setPhoneNumber(phone);
    customer->setArchived(archived);

    addCustomer(customer);
    return true;
}

bool HotelManager::restoreInvoiceFromDatabase(
    const std::string &invoiceId,
    const std::string &bookingId,
    double taxRate,
    int nights,
    const std::string &paymentDate,
    std::string &errorMessage)
{
    if (invoiceId.empty())
    {
        errorMessage = "Persisted invoice ID is empty.";
        return false;
    }

    if (invoiceIdExists(invoiceId))
    {
        errorMessage =
            "Duplicate persisted invoice ID: " + invoiceId;
        return false;
    }

    auto booking = findBookingById(bookingId);
    if (!booking)
    {
        errorMessage = "Booking not found for persisted invoice.";
        return false;
    }

    if (booking->isDeleted())
    {
        errorMessage = "Invoice can not be restored for a deleted booking.";
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

    if (!isValidDateString(paymentDate, errorMessage))
    {
        return false;
    }

    const BookingState state = getBookingState(*booking);

    if (state != BookingState::COMPLETED)
    {
        errorMessage = "Invoice can only be restored for a completed booking.";
        return false;
    }

    if (findInvoiceForBooking(bookingId) != nullptr)
    {
        errorMessage = "An invoice already exists for this booking.";
        return false;
    }

    auto invoice = std::make_shared<Invoice>();
    invoice->setInvoiceId(invoiceId);
    invoice->setBookingId(bookingId);
    invoice->setBooking(booking);
    invoice->setTaxRate(taxRate);
    invoice->setNights(nights);
    invoice->setPaymentDate(paymentDate);

    addInvoice(invoice);
    return true;
}
bool HotelManager::cancelBooking(const std::string &bookingId, std::string &errorMessage)
{
    return ReservationService(*this).cancelBooking(bookingId, errorMessage);
}

// =========================
// Delete methods
// =========================
// Added: Hard-deletes a room, customer, or invoice only if no active references exist
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
        if (!booking || !booking->getRoom())
            continue;

        if (booking->getRoom()->getRoomNumber() == roomNumber)
        {
            errorMessage = "Cannot delete room because it is referenced by booking history.";
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
        if (!booking || !booking->getCustomer())
            continue;

        if (booking->getCustomer()->getCustomerId() == customerId)
        {
            errorMessage = "Cannot delete customer because they are referenced by booking history.";
            return false;
        }
    }

    customers.erase(std::remove(customers.begin(), customers.end(), customer), customers.end());
    return true;
}

bool HotelManager::soft_deleteBooking(const std::string &bookingId, std::string &errorMessage)
{
    // Keep the booking record for history and audit purposes.
    auto booking = findBookingById(bookingId);
    if (!booking)
    {
        errorMessage = "Booking not found.";
        return false;
    }

    if (booking->isDeleted())
    {
        errorMessage = "Booking has already been deleted.";
        return false;
    }

    if (getBookingState(*booking) == BookingState::ACTIVE)
    {
        errorMessage = "Cannot delete an active booking.";
        return false;
    }

    booking->setDeleted(true);
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
