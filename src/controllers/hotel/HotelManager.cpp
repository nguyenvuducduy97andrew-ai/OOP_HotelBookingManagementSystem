#include "HotelManager.h"
#include "../booking/BookingManager.h"
#include "../customer/CustomerManager.h"
#include "../room/RoomManager.h"
#include <QDate>
#include <QList>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QUuid>
#include <algorithm>
#include <cctype>
#include <unordered_set>
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
    // Modified: Validate supported national ID formats before data reaches persistence.
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
    // Modified: Validate E.164-style numbers for selectable country dialing codes.
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
    roomMaintenances.clear();
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

const std::vector<RoomMaintenance>& HotelManager::getRoomMaintenances() const
{
    return roomMaintenances;
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
    const std::string today = QDate::currentDate().toString(Qt::ISODate).toStdString();
    std::unordered_set<std::string> occupiedRoomNumbers;
    for (const auto& booking : bookings) {
        if (!booking || booking->isCancelled() || booking->isDeleted()
            || getBookingState(*booking) != BookingState::ACTIVE) {
            continue;
        }

        const auto bookedRoom = booking->getRoom();
        if (bookedRoom) {
            occupiedRoomNumbers.insert(bookedRoom->getRoomNumber());
        }
    }

    std::vector<std::shared_ptr<Room>> availableRooms;
    availableRooms.reserve(rooms.size());
    for (const auto &room : rooms)
    {
        if (!room || !room->getIsAvailable() || room->isArchived()
            || isRoomUnderMaintenance(room->getRoomNumber(), today))
        {
            continue;
        }

        if (occupiedRoomNumbers.find(room->getRoomNumber()) == occupiedRoomNumbers.end())
        {
            availableRooms.push_back(room);
        }
    }

    // Modified: Calculate active occupancy once and include dated maintenance in the current-room availability query.
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

// Modified: Add query mappings that filter structural booking data into specialized layout sub-tabs.
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

// Modified: Add a query helper for dynamic dashboard timelines instead of a hardcoded system clock.
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

// Modified: Add a query helper for dashboard departure timeline tracking.
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

    // Modified: Make completion depend on the persisted checkout action instead of the planned departure date.
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

// Modified: Exclude cancelled and completed records when calculating active occupancy data.
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

bool HotelManager::registerRoomCore(
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

bool HotelManager::registerCustomerCore(
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

bool HotelManager::updateCustomerCore(
    const std::string& customerId,
    const std::string& name,
    const std::string& phone,
    std::string& errorMessage)
{
    const auto customer = findCustomerById(customerId);
    if (!customer) {
        errorMessage = "Customer not found.";
        return false;
    }
    if (!isValidCustomerNameFormat(name)) {
        errorMessage = "Customer name must have at least 2 words and include both uppercase and lowercase letters.";
        return false;
    }
    if (!isValidPhoneNumberFormat(phone)) {
        errorMessage = "Phone number does not match the selected country format.";
        return false;
    }

    // Modified: Keep customer identity immutable while validating all editable customer fields in the core layer.
    customer->setName(name);
    customer->setPhoneNumber(phone);
    return true;
}

bool HotelManager::resolveCustomerForBookingCore(
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

bool HotelManager::isRoomUnderMaintenance(const std::string& roomNumber, const std::string& date) const
{
    return std::any_of(roomMaintenances.cbegin(), roomMaintenances.cend(),
        [&roomNumber, &date](const RoomMaintenance& maintenance) {
            return maintenance.getRoomNumber() == roomNumber
                && maintenance.getStartDate() <= date
                && date < maintenance.getEndDate();
        });
}

bool HotelManager::hasRoomMaintenanceConflict(const std::string& roomNumber,
                                              const std::string& startDate,
                                              const std::string& endDate,
                                              std::string& errorMessage) const
{
    const QDate start = QDate::fromString(QString::fromStdString(startDate), Qt::ISODate);
    const QDate end = QDate::fromString(QString::fromStdString(endDate), Qt::ISODate);
    if (!start.isValid() || !end.isValid() || end <= start) {
        errorMessage = "Maintenance availability requires a valid ISO date range.";
        return true;
    }

    for (const RoomMaintenance& maintenance : roomMaintenances) {
        if (maintenance.getRoomNumber() != roomNumber) {
            continue;
        }

        // Modified: Use the same half-open interval rule as bookings, allowing a check-in on the maintenance end date.
        if (startDate < maintenance.getEndDate() && maintenance.getStartDate() < endDate) {
            errorMessage = "Room " + roomNumber + " is under maintenance from "
                + maintenance.getStartDate() + " to " + maintenance.getEndDate() + ".";
            return true;
        }
    }

    return false;
}

bool HotelManager::registerRoom(RoomType kind, const std::string& roomNumber, double baseRate, std::string& errorMessage)
{
    // Modified: Keep the hotel facade one level above the room workflow manager.
    return RoomManager(*this).registerRoom(kind, roomNumber, baseRate, errorMessage);
}

bool HotelManager::registerCustomer(const std::string& id, const std::string& name, const std::string& phone, std::string& errorMessage, std::string* conflictingCustomerId)
{
    return CustomerManager(*this).registerCustomer(id, name, phone, errorMessage, conflictingCustomerId);
}

bool HotelManager::updateCustomer(const std::string& customerId, const std::string& name, const std::string& phone,
                                  std::string& errorMessage, std::string* conflictingCustomerId)
{
    return CustomerManager(*this).updateCustomer(customerId, name, phone, errorMessage, conflictingCustomerId);
}

bool HotelManager::resolveCustomerForBooking(const std::string& id, const std::string& name, const std::string& phone, std::string& errorMessage)
{
    return CustomerManager(*this).resolveForBooking(id, name, phone, errorMessage);
}

bool HotelManager::createBooking(
    const std::string &customerId,
    const std::string &roomNumber,
    const std::string &checkIn,
    const std::string &checkOut,
    std::string &errorMessage)
{
    // Modified: Delegate booking creation to the focused service while preserving the existing UI-facing API.
    return BookingManager(*this).createBooking(customerId, roomNumber, checkIn, checkOut, errorMessage);
}

bool HotelManager::updateBooking(
    const std::string &bookingId,
    const std::string &customerId,
    const std::string &roomNumber,
    const std::string &checkInDate,
    const std::string &checkOutDate,
    std::string &errorMessage)
{
    return BookingManager(*this).updateBooking(
        bookingId, customerId, roomNumber, checkInDate, checkOutDate, errorMessage);
}

bool HotelManager::completeBooking(
    const std::string &bookingId,
    const std::string &actualCheckoutDate,
    std::string &errorMessage)
{
    return BookingManager(*this).completeBooking(bookingId, actualCheckoutDate, errorMessage);
}

bool HotelManager::createInvoice( // In practice, checkout first, then create invoice for completed booking
    const std::string &invoiceId,
    const std::string &bookingId,
    double taxRate,
    int nights, // Intentionally for partial stays
    const std::string &paymentDate,
    std::string &errorMessage)
{
    // Modified: Keep invoice validation and creation isolated from the in-memory hotel store.
    return BookingManager(*this).createInvoice(invoiceId, bookingId, taxRate, nights, paymentDate, errorMessage);
}

bool HotelManager::setRoomAvailabilityCore(
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

            const BookingState state = getBookingState(*booking);
            // Modified: Block maintenance whenever the room has an unfinished booking, including future arrivals.
            if (state == BookingState::UPCOMING || state == BookingState::ACTIVE)
            {
                errorMessage = "Cannot place this room under maintenance while it has an active or upcoming booking.";
                return false;
            }
        }
    }

    room->setIsAvailable(available);
    return true;
}

// Modified: Archive a room or customer without deleting historical data.
bool HotelManager::archiveRoomCore(const std::string &roomNumber, std::string &errorMessage)
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

        const BookingState state = getBookingState(*booking);
        // Modified: Preserve room and guest references for every unfinished stay, including future arrivals.
        if (state == BookingState::UPCOMING || state == BookingState::ACTIVE)
        {
            errorMessage = "Cannot archive room while it has an active or upcoming booking.";
            return false;
        }
    }

    room->setArchived(true);
    return true;
}

bool HotelManager::archiveCustomerCore(const std::string &customerId, std::string &errorMessage)
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

        const BookingState state = getBookingState(*booking);
        // Modified: Prevent archival of a customer referenced by an unfinished booking.
        if (state == BookingState::UPCOMING || state == BookingState::ACTIVE)
        {
            errorMessage = "Cannot archive customer while they have an active or upcoming booking.";
            return false;
        }
    }

    customer->setArchived(true);
    return true;
}

// Modified: Unarchive a room or customer to restore it to active listings.
bool HotelManager::restoreRoomCore(const std::string &roomNumber, std::string &errorMessage)
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

bool HotelManager::restoreCustomerCore(const std::string &customerId, std::string &errorMessage)
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

bool HotelManager::scheduleRoomMaintenanceCore(const std::string& roomNumber,
                                               const std::string& startDate,
                                               const std::string& endDate,
                                               const std::string& note,
                                               std::string& errorMessage)
{
    const auto room = findRoomByNumber(roomNumber);
    if (!room) {
        errorMessage = "Room not found.";
        return false;
    }
    if (room->isArchived()) {
        errorMessage = "Cannot schedule maintenance for an archived room.";
        return false;
    }

    const QDate start = QDate::fromString(QString::fromStdString(startDate), Qt::ISODate);
    const QDate end = QDate::fromString(QString::fromStdString(endDate), Qt::ISODate);
    if (!start.isValid() || !end.isValid() || end <= start) {
        errorMessage = "Maintenance end date must be after the start date.";
        return false;
    }

    if (hasRoomMaintenanceConflict(roomNumber, startDate, endDate, errorMessage)) {
        return false;
    }

    for (const auto& booking : bookings) {
        if (!booking || booking->isCancelled() || booking->isDeleted() ||
            !booking->getRoom() || booking->getRoom()->getRoomNumber() != roomNumber ||
            getBookingState(*booking) == BookingState::COMPLETED) {
            continue;
        }

        if (startDate < booking->getCheckOutDate() && booking->getCheckInDate() < endDate) {
            errorMessage = "Maintenance conflicts with booking " + booking->getBookingId()
                + " (" + booking->getCheckInDate() + " to " + booking->getCheckOutDate()
                + "). Reassign, reschedule, or cancel that booking first.";
            return false;
        }
    }

    // Modified: Represent maintenance as a persisted date interval instead of a room-wide boolean lock.
    const std::string maintenanceId = "MTN-" + QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    roomMaintenances.emplace_back(maintenanceId, roomNumber, startDate, endDate, note);
    return true;
}

bool HotelManager::cancelRoomMaintenanceCore(const std::string& maintenanceId, std::string& errorMessage)
{
    const auto matchingMaintenance = std::find_if(roomMaintenances.cbegin(), roomMaintenances.cend(),
        [&maintenanceId](const RoomMaintenance& maintenance) {
            return maintenance.getMaintenanceId() == maintenanceId;
        });
    if (matchingMaintenance == roomMaintenances.cend()) {
        errorMessage = "Maintenance schedule not found.";
        return false;
    }

    // Modified: Remove one maintenance interval without changing the room's permanent availability setting.
    roomMaintenances.erase(matchingMaintenance);
    return true;
}

bool HotelManager::setRoomAvailability(const std::string& roomNumber, bool available, std::string& errorMessage)
{
    return RoomManager(*this).setAvailability(roomNumber, available, errorMessage);
}

bool HotelManager::scheduleRoomMaintenance(const std::string& roomNumber, const std::string& startDate,
                                           const std::string& endDate, const std::string& note,
                                           std::string& errorMessage)
{
    return RoomManager(*this).scheduleMaintenance(roomNumber, startDate, endDate, note, errorMessage);
}

bool HotelManager::cancelRoomMaintenance(const std::string& maintenanceId, std::string& errorMessage)
{
    return RoomManager(*this).cancelMaintenance(maintenanceId, errorMessage);
}

bool HotelManager::archiveRoom(const std::string& roomNumber, std::string& errorMessage)
{
    return RoomManager(*this).archiveRoom(roomNumber, errorMessage);
}

bool HotelManager::restoreRoom(const std::string& roomNumber, std::string& errorMessage)
{
    return RoomManager(*this).restoreRoom(roomNumber, errorMessage);
}

bool HotelManager::archiveCustomer(const std::string& customerId, std::string& errorMessage)
{
    return CustomerManager(*this).archiveCustomer(customerId, errorMessage);
}

bool HotelManager::restoreCustomer(const std::string& customerId, std::string& errorMessage)
{
    return CustomerManager(*this).restoreCustomer(customerId, errorMessage);
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
    // Modified: Restore completed same-day stays from SQLite while rejecting only a checkout before check-in.
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

    if (!isValidCustomerIdFormat(customerId) || !isValidCustomerNameFormat(name) || !isValidPhoneNumberFormat(phone))
    {
        errorMessage = "Persisted customer record has an invalid ID, name, or phone number.";
        return false;
    }

    for (const auto& existing : customers) {
        if (existing && existing->getPhoneNumber() == phone) {
            errorMessage = "Duplicate persisted customer phone number: " + phone;
            return false;
        }
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
    double unitPrice,
    const std::string &customerNameSnapshot,
    const std::string &customerIdSnapshot,
    const std::string &customerPhoneSnapshot,
    const std::string &roomNumberSnapshot,
    const std::string &roomTypeSnapshot,
    const std::string &checkInDateSnapshot,
    const std::string &checkOutDateSnapshot,
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

    if (taxRate < 0 || taxRate > 1)
    {
        errorMessage = "Tax rate must be between 0% and 100%.";
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

    if (unitPrice <= 0 || customerNameSnapshot.empty() || customerIdSnapshot.empty() || customerPhoneSnapshot.empty() ||
        roomNumberSnapshot.empty() || roomTypeSnapshot.empty() ||
        !isValidDateString(checkInDateSnapshot, errorMessage) ||
        !isValidDateString(checkOutDateSnapshot, errorMessage))
    {
        errorMessage = "Persisted invoice snapshot is incomplete or invalid.";
        return false;
    }
    const QDate payment = QDate::fromString(QString::fromStdString(paymentDate), Qt::ISODate);
    const QDate snapshotCheckout = QDate::fromString(QString::fromStdString(checkOutDateSnapshot), Qt::ISODate);
    if (payment < snapshotCheckout || payment > QDate::currentDate()) {
        errorMessage = "Persisted invoice payment date is outside the allowed range.";
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
    invoice->setUnitPrice(unitPrice);
    invoice->setCustomerNameSnapshot(customerNameSnapshot);
    invoice->setCustomerIdSnapshot(customerIdSnapshot);
    invoice->setCustomerPhoneSnapshot(customerPhoneSnapshot);
    invoice->setRoomNumberSnapshot(roomNumberSnapshot);
    invoice->setRoomTypeSnapshot(roomTypeSnapshot);
    invoice->setCheckInDateSnapshot(checkInDateSnapshot);
    invoice->setCheckOutDateSnapshot(checkOutDateSnapshot);
    if (!invoice->isValid()) {
        errorMessage = "Persisted invoice is invalid.";
        return false;
    }

    addInvoice(invoice);
    return true;
}

bool HotelManager::restoreRoomMaintenanceFromDatabase(const std::string& maintenanceId,
                                                      const std::string& roomNumber,
                                                      const std::string& startDate,
                                                      const std::string& endDate,
                                                      const std::string& note,
                                                      std::string& errorMessage)
{
    if (maintenanceId.empty()) {
        errorMessage = "Persisted maintenance ID is empty.";
        return false;
    }
    if (!findRoomByNumber(roomNumber)) {
        errorMessage = "Persisted maintenance references a missing room.";
        return false;
    }

    const QDate start = QDate::fromString(QString::fromStdString(startDate), Qt::ISODate);
    const QDate end = QDate::fromString(QString::fromStdString(endDate), Qt::ISODate);
    if (!start.isValid() || !end.isValid() || end <= start) {
        errorMessage = "Persisted maintenance dates are invalid.";
        return false;
    }

    if (std::any_of(roomMaintenances.cbegin(), roomMaintenances.cend(),
                    [&maintenanceId](const RoomMaintenance& maintenance) {
                        return maintenance.getMaintenanceId() == maintenanceId;
                    })) {
        errorMessage = "Duplicate persisted maintenance ID: " + maintenanceId;
        return false;
    }

    if (hasRoomMaintenanceConflict(roomNumber, startDate, endDate, errorMessage)) {
        return false;
    }

    for (const auto& booking : bookings) {
        if (!booking || booking->isCancelled() || booking->isDeleted() ||
            !booking->getRoom() || booking->getRoom()->getRoomNumber() != roomNumber ||
            getBookingState(*booking) == BookingState::COMPLETED) {
            continue;
        }

        if (startDate < booking->getCheckOutDate() && booking->getCheckInDate() < endDate) {
            errorMessage = "Persisted maintenance conflicts with unfinished booking " + booking->getBookingId() + ".";
            return false;
        }
    }

    // Modified: Reject persisted maintenance that violates the same overlap rules used by live scheduling.
    roomMaintenances.emplace_back(maintenanceId, roomNumber, startDate, endDate, note);
    return true;
}

bool HotelManager::cancelBooking(const std::string &bookingId, std::string &errorMessage)
{
    return BookingManager(*this).cancelBooking(bookingId, errorMessage);
}

// =========================
// Delete methods
// =========================
// Modified: Hard-delete a room, customer, or invoice only when no active references exist.
bool HotelManager::deleteRoom(const std::string& roomNumber, std::string& errorMessage)
{
    return RoomManager(*this).deleteRoom(roomNumber, errorMessage);
}

bool HotelManager::deleteCustomer(const std::string& customerId, std::string& errorMessage)
{
    return CustomerManager(*this).deleteCustomer(customerId, errorMessage);
}

bool HotelManager::deleteRoomCore(const std::string &roomNumber, std::string &errorMessage)
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

bool HotelManager::deleteCustomerCore(const std::string &customerId, std::string &errorMessage)
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
