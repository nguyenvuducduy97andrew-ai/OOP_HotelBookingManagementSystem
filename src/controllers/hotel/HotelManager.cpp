#include "HotelManager.h"
#include "BookingManager.h"
#include "CustomerManager.h"
#include "RoomManager.h"
#include <QDate>
#include <QDateTime>
#include <QTime>
#include <QList>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include "CustomerIdentity.h"
#include "customer/CountryInputRules.h"
#include <QUuid>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <unordered_set>
#include <type_traits>
#include <utility>

static_assert(std::is_move_constructible_v<HotelManager>);
static_assert(std::is_move_assignable_v<HotelManager>);

namespace {
std::string collapseWhitespace(const std::string& value)
{
    return QString::fromStdString(value).simplified().toStdString();
}

QDateTime plannedBookingBoundary(const Booking& booking, bool checkIn)
{
    // Modified: Prefer canonical planned timestamps while retaining a midnight fallback only for imported legacy records.
    const std::string& timestamp = checkIn ? booking.getPlannedCheckInAt() : booking.getPlannedCheckOutAt();
    QDateTime boundary = QDateTime::fromString(QString::fromStdString(timestamp), Qt::ISODateWithMs);
    if (!boundary.isValid()) {
        boundary = QDateTime::fromString(QString::fromStdString(timestamp), Qt::ISODate);
    }
    if (!boundary.isValid()) {
        const std::string& legacyDate = checkIn ? booking.getCheckInDate() : booking.getCheckOutDate();
        const QDate parsedDate = QDate::fromString(QString::fromStdString(legacyDate), Qt::ISODate);
        boundary = parsedDate.isValid() ? QDateTime(parsedDate, QTime(0, 0)) : QDateTime();
    }
    return boundary;
}

bool isSingleNameTokenValid(const QString& token)
{
    static const QRegularExpression tokenPattern(QStringLiteral(R"(^[\p{L}][\p{L}'’\-.]*$)"));
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
    case BookingState::NO_SHOW:
        return "Cancelled";
    default:
        return "Unknown";
    }
}

HotelManager::HotelManager() = default;

bool HotelManager::isValidCustomerIdFormat(const std::string& customerId)
{
    // Modified: Validate the document type, issuing country, and number stored in the international customer identity key.
    const QString id = QString::fromStdString(customerId).trimmed().toUpper();
    const QStringList identityParts = id.split('|');
    if (identityParts.size() == 3) {
        return isValidDocumentNumber(identityParts.at(0), identityParts.at(1), identityParts.at(2));
    }
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
    // Modified: Accept international legal names, including mononyms, uppercase document names, caseless scripts, and initials.
    const QString normalized = QString::fromStdString(collapseWhitespace(customerName));
    if (normalized.isEmpty() || normalized.size() > 120) {
        return false;
    }

    const QStringList tokens = normalized.split(' ', Qt::SkipEmptyParts);
    if (tokens.isEmpty()) {
        return false;
    }

    for (const QString& token : tokens) {
        if (!isSingleNameTokenValid(token)) {
            return false;
        }
    }
    return true;
}

bool HotelManager::isValidPhoneNumberFormat(const std::string& phoneNumber)
{
    // Modified: Validate the E.164 envelope while dialogs enforce the documented supported range for their selected country profile.
    const QString phone = QString::fromStdString(collapseWhitespace(phoneNumber));
    static const QRegularExpression phonePattern(QStringLiteral(R"(^\+[1-9]\d{7,14}$)"));
    return phonePattern.match(phone).hasMatch();
}

void HotelManager::clearAll()
{
    m_roomManager.clearAll();
    m_customerManager.clearAll();
    m_bookingManager.clearAll();
}

// =========================
// Getters
// =========================

const std::vector<std::shared_ptr<Room>> &HotelManager::getRooms() const
{
    return m_roomManager.getRooms();
}

const std::vector<std::shared_ptr<Customer>> &HotelManager::getCustomers() const
{
    return m_customerManager.getCustomers();
}

const std::vector<std::shared_ptr<Booking>> &HotelManager::getBookings() const
{
    return m_bookingManager.getBookings();
}

const std::vector<std::shared_ptr<Invoice>> &HotelManager::getInvoices() const
{
    return m_bookingManager.getInvoices();
}

const std::vector<RoomMaintenance>& HotelManager::getRoomMaintenances() const
{
    return m_roomManager.getRoomMaintenances();
}

const std::vector<MaintenanceGuestNotice>& HotelManager::getMaintenanceGuestNotices() const
{
    return m_roomManager.getMaintenanceGuestNotices();
}

std::vector<MaintenanceGuestNotice> HotelManager::getMaintenanceGuestNotices(const std::string& maintenanceId) const
{
    return m_roomManager.getMaintenanceGuestNotices(maintenanceId);
}


// =========================
// Validation helpers
// =========================

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
    return m_roomManager.roomNumberExists(roomNumber);
}

bool HotelManager::customerIdExists(const std::string &customerId) const
{
    return m_customerManager.customerIdExists(customerId);
}

bool HotelManager::bookingIdExists(const std::string &bookingId) const
{
    return m_bookingManager.bookingIdExists(bookingId);
}

bool HotelManager::invoiceIdExists(const std::string &invoiceId) const
{
    return m_bookingManager.invoiceIdExists(invoiceId);
}

// =========================
// Find methods
// =========================

std::shared_ptr<Room> HotelManager::findRoomByNumber(const std::string &roomNumber) const
{
    return m_roomManager.findRoomByNumber(roomNumber);
}

std::shared_ptr<Customer> HotelManager::findCustomerById(const std::string &customerId) const
{
    return m_customerManager.findCustomerById(customerId);
}

std::shared_ptr<Booking> HotelManager::findBookingById(const std::string &bookingId) const
{
    return m_bookingManager.findBookingById(bookingId);
}

std::shared_ptr<Invoice> HotelManager::findInvoiceById(const std::string &invoiceId) const
{
    return m_bookingManager.findInvoiceById(invoiceId);
}

std::shared_ptr<Invoice> HotelManager::findInvoiceForBooking(const std::string &bookingId) const
{
    return m_bookingManager.findInvoiceForBooking(bookingId);
}
// =========================
// Queries
// =========================

std::vector<std::shared_ptr<Room>> HotelManager::getAvailableRooms() const
{
    const std::string now = QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toStdString();
    std::unordered_set<std::string> occupiedRoomNumbers;
    for (const auto& booking : m_bookingManager.getBookings()) {
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
    availableRooms.reserve(m_roomManager.getRooms().size());
    for (const auto &room : m_roomManager.getRooms())
    {
        if (!room || !room->getIsAvailable() || room->isArchived()
            || m_roomManager.isRoomBlockedAt(room->getRoomNumber(), now))
        {
            continue;
        }

        if (occupiedRoomNumbers.find(room->getRoomNumber()) == occupiedRoomNumbers.end())
        {
            availableRooms.push_back(room);
        }
    }

    // Modified: Calculate active occupancy once and evaluate Cleaning or Maintenance at the current timestamp, not as an all-day date flag.
    return availableRooms;
}

std::vector<std::shared_ptr<Booking>> HotelManager::getBookingsForCustomer(const std::string &customerId) const
{
    return m_bookingManager.getBookingsForCustomer(customerId);
}

// Modified: Add query mappings that filter structural booking data into specialized layout sub-tabs.
std::vector<std::shared_ptr<Booking>> HotelManager::getBookingsByStatus(BookingState state) const
{
    return m_bookingManager.getBookingsByStatus(state);
}

BookingState HotelManager::getBookingState(const Booking &booking) const
{
    return m_bookingManager.getBookingState(booking);
}

// Modified: Exclude cancelled and completed records when calculating active occupancy data.
std::vector<std::shared_ptr<Room>> HotelManager::getRoomsByOccupancy(bool occupied) const
{
    std::vector<std::shared_ptr<Room>> matchingRooms;

    for (const auto &room : m_roomManager.getRooms())
    {
        if (!room)
            continue;

        bool isOccupied = false;
        for (const auto &booking : m_bookingManager.getBookings())
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
    return m_bookingManager.nextInvoiceId();
}

// =========================
// Use-case methods
// =========================

bool HotelManager::isRoomUnderMaintenance(const std::string& roomNumber, const std::string& date) const
{
    return m_roomManager.isRoomUnderMaintenance(roomNumber, date);
}

std::vector<std::shared_ptr<Room>> HotelManager::getAvailableRoomsForPeriod(
    const std::string& plannedCheckInAt,
    const std::string& plannedCheckOutAt,
    std::string& errorMessage,
    const std::string& excludedBookingId) const
{
    return m_bookingManager.getAvailableRoomsForPeriod(
        plannedCheckInAt, plannedCheckOutAt, m_roomManager.getRooms(),
        m_roomManager.getRoomMaintenances(), errorMessage, excludedBookingId);
}

bool HotelManager::isRoomBlockedAt(const std::string& roomNumber, const std::string& at) const
{
    return m_roomManager.isRoomBlockedAt(roomNumber, at);
}

std::vector<std::string> HotelManager::getCheckoutConflictWarnings(const std::string& bookingId,
                                                                    const std::string& actualCheckoutAt) const
{
    std::vector<std::string> warnings;
    const auto booking = m_bookingManager.findBookingById(bookingId);
    const QDateTime checkoutAt = QDateTime::fromString(QString::fromStdString(actualCheckoutAt), Qt::ISODateWithMs);
    if (!booking || !booking->getRoom() || !checkoutAt.isValid()) {
        return warnings;
    }

    QDateTime plannedDeparture = QDateTime::fromString(
        QString::fromStdString(booking->getPlannedCheckOutAt()), Qt::ISODateWithMs);
    if (!plannedDeparture.isValid()) {
        const QDate legacyDate = QDate::fromString(QString::fromStdString(booking->getCheckOutDate()), Qt::ISODate);
        plannedDeparture = legacyDate.isValid() ? QDateTime(legacyDate, QTime(0, 0)) : QDateTime();
    }
    if (plannedDeparture.isValid() && checkoutAt > plannedDeparture.addSecs(30 * 60)) {
        // Modified: Identify the room and both times so staff can judge a late checkout without guessing its operational impact.
        warnings.push_back("Late checkout warning — Room " + booking->getRoom()->getRoomNumber()
            + ": planned " + plannedDeparture.toString(Qt::ISODate).toStdString()
            + ", actual " + checkoutAt.toString(Qt::ISODate).toStdString()
            + ". Actual duration will be billed using the hourly rounding rule.");
    }

    const QDateTime cleaningEnd = checkoutAt.addSecs(2 * 60 * 60);
    for (const auto& candidate : m_bookingManager.getBookings()) {
        if (!candidate || candidate->getBookingId() == bookingId || candidate->isCancelled()
            || candidate->isDeleted() || !candidate->getRoom()
            || candidate->getRoom()->getRoomNumber() != booking->getRoom()->getRoomNumber()
            || m_bookingManager.getBookingState(*candidate) != BookingState::UPCOMING) {
            continue;
        }
        QDateTime plannedArrival = QDateTime::fromString(
            QString::fromStdString(candidate->getPlannedCheckInAt()), Qt::ISODateWithMs);
        if (!plannedArrival.isValid()) {
            const QDate legacyDate = QDate::fromString(QString::fromStdString(candidate->getCheckInDate()), Qt::ISODate);
            plannedArrival = legacyDate.isValid() ? QDateTime(legacyDate, QTime(0, 0)) : QDateTime();
        }
        if (plannedArrival.isValid() && plannedArrival < cleaningEnd) {
            warnings.push_back("Arrival at risk — Room " + booking->getRoom()->getRoomNumber()
                + ": booking " + candidate->getBookingId() + " is planned to arrive at "
                + plannedArrival.toString(Qt::ISODate).toStdString() + ", but Cleaning after this checkout is expected to finish at "
                + cleaningEnd.toString(Qt::ISODate).toStdString() + ".");
        }
    }
    // Modified: Keep late-checkout and turnover conflicts visible to staff without silently overriding the next reservation.
    return warnings;
}

bool HotelManager::hasRoomMaintenanceConflict(const std::string& roomNumber,
                                              const std::string& startDate,
                                              const std::string& endDate,
                                              std::string& errorMessage) const
{
    return m_roomManager.hasRoomMaintenanceConflict(roomNumber, startDate, endDate, errorMessage);
}

bool HotelManager::registerRoom(RoomType kind, const std::string& roomNumber, double baseRate, 
                                double area, const std::string& bedType, int maxGuests, 
                                const std::string& description, const std::string& amenities, 
                                std::string& errorMessage)
{
    if (area <= 0) {
        errorMessage = "Area must be greater than zero.";
        return false;
    }
    if (maxGuests <= 0) {
        errorMessage = "Max guests must be at least 1.";
        return false;
    }
    return m_roomManager.registerRoom(kind, roomNumber, baseRate, area, bedType, maxGuests, description, amenities, errorMessage);
}

bool HotelManager::updateRoomDetails(const std::string& roomNumber, double baseRate, double extraFee,
                                     double area, const std::string& bedType, int maxGuests,
                                     const std::string& description, const std::string& amenities,
                                     std::string& errorMessage)
{
    if (!std::isfinite(baseRate) || baseRate <= 0.0) {
        errorMessage = "Base rate must be a finite value greater than zero.";
        return false;
    }
    if (!std::isfinite(extraFee) || extraFee < 0.0) {
        errorMessage = "Extra fee must be a finite non-negative value.";
        return false;
    }
    if (area <= 15) {
        errorMessage = "Area must be greater than 15. No room should be that small!";
        return false;
    }
    if (maxGuests <= 0) {
        errorMessage = "Max guests must be at least 1.";
        return false;
    }

    return m_roomManager.updateRoomDetails(roomNumber, baseRate, extraFee, area, bedType, maxGuests, description, amenities, errorMessage);
}

bool HotelManager::registerCustomer(const std::string& id, const std::string& name, const std::string& phone, std::string& errorMessage, std::string* conflictingCustomerId)
{
    return m_customerManager.registerCustomer(id, name, phone, errorMessage, conflictingCustomerId);
}

bool HotelManager::updateCustomer(const std::string& customerId, const std::string& name, const std::string& phone,
                                  std::string& errorMessage, std::string* conflictingCustomerId)
{
    return m_customerManager.updateCustomer(customerId, name, phone, errorMessage, conflictingCustomerId);
}

bool HotelManager::resolveCustomerForBooking(const std::string& id, const std::string& name, const std::string& phone, std::string& errorMessage)
{
    return m_customerManager.resolveForBooking(id, name, phone, errorMessage);
}

bool HotelManager::createBooking(
    const std::string &customerId,
    const std::string &roomNumber,
    const std::string &checkIn,
    const std::string &checkOut,
    int adultCount,
    int childCount,
    std::string &errorMessage)
{
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

    return m_bookingManager.createBookingResolved(customer, room, checkIn, checkOut,
                                                  adultCount, childCount,
                                                  m_roomManager.getRoomMaintenances(),
                                                  errorMessage);
}

bool HotelManager::updateBooking(
    const std::string &bookingId,
    const std::string &customerId,
    const std::string &roomNumber,
    const std::string &checkInDate,
    const std::string &checkOutDate,
    int adultCount,
    int childCount,
    std::string &errorMessage)
{
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

    return m_bookingManager.updateBookingResolved(bookingId, customerId, roomNumber, customer, room, checkInDate, checkOutDate,
                                                  adultCount, childCount, m_roomManager.getRoomMaintenances(),
                                                  errorMessage);
}

bool HotelManager::createBookingAt(const std::string& customerId,
                                   const std::string& roomNumber,
                                   const std::string& plannedCheckInAt,
                                   const std::string& plannedCheckOutAt,
                                   int adultCount,
                                   int childCount,
                                   std::string& errorMessage)
{
    const auto customer = findCustomerById(customerId);
    const auto room = findRoomByNumber(roomNumber);
    if (!customer || !room) {
        errorMessage = "Customer or room not found.";
        return false;
    }
    return m_bookingManager.createBookingAtResolved(
        customer, room, plannedCheckInAt, plannedCheckOutAt, adultCount, childCount,
        m_roomManager.getRoomMaintenances(), errorMessage);
}

bool HotelManager::createBookingForNewCustomer(const std::string& customerId,
                                               const std::string& customerName,
                                               const std::string& customerPhone,
                                               const std::string& roomNumber,
                                               const std::string& plannedCheckInAt,
                                               const std::string& plannedCheckOutAt,
                                               int adultCount,
                                               int childCount,
                                               std::string& errorMessage)
{
    // Modified: Register a new guest only as part of a successful reservation attempt, rolling it back when booking validation fails to avoid orphan customer records.
    if (!m_customerManager.registerCustomer(customerId, customerName, customerPhone, errorMessage)) {
        return false;
    }
    if (createBookingAt(customerId, roomNumber, plannedCheckInAt, plannedCheckOutAt,
                        adultCount, childCount, errorMessage)) {
        return true;
    }

    std::string rollbackError;
    if (!m_customerManager.deleteCustomer(customerId, rollbackError)) {
        errorMessage += " The new customer could not be rolled back: " + rollbackError;
    }
    return false;
}

bool HotelManager::updateBookingAt(const std::string& bookingId,
                                   const std::string& customerId,
                                   const std::string& roomNumber,
                                   const std::string& plannedCheckInAt,
                                   const std::string& plannedCheckOutAt,
                                   int adultCount,
                                   int childCount,
                                   std::string& errorMessage)
{
    const auto customer = findCustomerById(customerId);
    const auto room = findRoomByNumber(roomNumber);
    if (!customer || !room) {
        errorMessage = "Customer or room not found.";
        return false;
    }
    // Modified: The facade exposes one timestamp scheduling path so future views cannot bypass room-block validation.
    return m_bookingManager.updateBookingAtResolved(
        bookingId, customer, room, plannedCheckInAt, plannedCheckOutAt, adultCount, childCount,
        m_roomManager.getRoomMaintenances(), errorMessage);
}

bool HotelManager::extendActiveBookingAt(const std::string& bookingId,
                                         const std::string& plannedCheckOutAt,
                                         std::string& errorMessage)
{
    // Modified: Keep the active-stay extension path narrow so it cannot be used as a general post-check-in edit.
    return m_bookingManager.extendActiveBookingAt(
        bookingId, plannedCheckOutAt, m_roomManager.getRoomMaintenances(), errorMessage);
}

bool HotelManager::completeBooking(
    const std::string &bookingId,
    const std::string &actualCheckoutDate,
    std::string &errorMessage)
{
    if (!m_bookingManager.completeBooking(bookingId, actualCheckoutDate, errorMessage)) {
        return false;
    }

    const auto booking = m_bookingManager.findBookingById(bookingId);
    if (!booking || !booking->getRoom()) {
        errorMessage = "Checkout completed but its room record is unavailable.";
        return false;
    }
    const std::string checkoutAt = booking->getActualCheckOutAt().empty()
        ? actualCheckoutDate + "T" + QTime::currentTime().toString("HH:mm:ss").toStdString()
        : booking->getActualCheckOutAt();
    // Modified: Checkout immediately creates the two-hour Cleaning block that protects the next guest's turnover window.
    if (m_roomManager.startCleaningAfterCheckout(booking->getRoom()->getRoomNumber(), checkoutAt, errorMessage)) {
        return true;
    }
    std::string rollbackError;
    if (!m_bookingManager.revertCompletedBooking(bookingId, rollbackError)) {
        errorMessage += " Checkout rollback also failed: " + rollbackError;
    }
    return false;
}

bool HotelManager::completeBookingAt(const std::string& bookingId, const std::string& actualCheckoutAt,
                                     std::string& errorMessage)
{
    if (!m_bookingManager.completeBookingAt(bookingId, actualCheckoutAt, errorMessage)) {
        return false;
    }
    const auto booking = m_bookingManager.findBookingById(bookingId);
    if (!booking || !booking->getRoom()) {
        errorMessage = "Checkout completed but its room record is unavailable.";
        return false;
    }
    // Modified: The timestamp-based checkout facade creates Cleaning from the exact recorded checkout moment.
    if (m_roomManager.startCleaningAfterCheckout(
            booking->getRoom()->getRoomNumber(), booking->getActualCheckOutAt(), errorMessage)) {
        return true;
    }
    std::string rollbackError;
    if (!m_bookingManager.revertCompletedBooking(bookingId, rollbackError)) {
        errorMessage += " Checkout rollback also failed: " + rollbackError;
    }
    return false;
}

bool HotelManager::checkInBooking(const std::string& bookingId, const std::string& checkInDate,
                                  std::string& errorMessage)
{
    const auto booking = m_bookingManager.findBookingById(bookingId);
    if (!booking || !booking->getRoom()) {
        errorMessage = "Booking or room not found.";
        return false;
    }
    const std::string checkInAt = checkInDate + "T" + QTime::currentTime().toString("HH:mm:ss").toStdString();
    if (m_roomManager.isRoomBlockedAt(booking->getRoom()->getRoomNumber(), checkInAt)) {
        errorMessage = "This room is still under Cleaning or Maintenance and is not ready for check-in.";
        return false;
    }
    for (const auto& other : m_bookingManager.getBookings()) {
        if (!other || other->getBookingId() == bookingId || !other->getRoom()
            || other->getRoom()->getRoomNumber() != booking->getRoom()->getRoomNumber()) {
            continue;
        }
        if (m_bookingManager.getBookingState(*other) == BookingState::ACTIVE) {
            errorMessage = "This room still has another active stay.";
            return false;
        }
    }
    // Modified: Check-in is allowed only when the room has no active stay and no operational block at the recorded moment.
    return m_bookingManager.checkInBooking(bookingId, checkInDate, errorMessage);
}

bool HotelManager::checkInBookingAt(const std::string& bookingId, const std::string& actualCheckInAt,
                                    std::string& errorMessage)
{
    const auto booking = m_bookingManager.findBookingById(bookingId);
    if (!booking || !booking->getRoom()) {
        errorMessage = "Booking or room not found.";
        return false;
    }
    if (m_roomManager.isRoomBlockedAt(booking->getRoom()->getRoomNumber(), actualCheckInAt)) {
        errorMessage = "This room is still under Cleaning or Maintenance and is not ready for check-in.";
        return false;
    }
    for (const auto& other : m_bookingManager.getBookings()) {
        if (other && other->getBookingId() != bookingId && other->getRoom()
            && other->getRoom()->getRoomNumber() == booking->getRoom()->getRoomNumber()
            && m_bookingManager.getBookingState(*other) == BookingState::ACTIVE) {
            errorMessage = "This room still has another active stay.";
            return false;
        }
    }
    // Modified: This timestamp facade allows early arrival only after the room's blocking operational work has ended.
    return m_bookingManager.checkInBookingAt(bookingId, actualCheckInAt, errorMessage);
}

bool HotelManager::markRoomReady(const std::string& roomNumber, const std::string& completedBy,
                                 std::string& errorMessage)
{
    // Modified: Reception can release a completed Cleaning block early while the original planned duration stays auditable.
    return m_roomManager.markRoomReady(
        roomNumber, QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toStdString(), completedBy, errorMessage);
}

bool HotelManager::createInvoice( // In practice, checkout first, then create invoice for completed booking
    const std::string &invoiceId,
    const std::string &bookingId,
    const std::string &invoiceIssuedDate,
    const std::string& paymentMethod,
    double paymentAmount,
    const std::string& paymentReceivedDate,
    std::string &errorMessage)
{
    return m_bookingManager.createInvoice(invoiceId, bookingId, invoiceIssuedDate,
                                          paymentMethod, paymentAmount, paymentReceivedDate, errorMessage);
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
        for (const auto &booking : m_bookingManager.getBookings())
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

    return m_roomManager.setRoomAvailability(roomNumber, available, errorMessage);
}

// Modified: Archive a room or customer without deleting historical data.
bool HotelManager::scheduleRoomMaintenance(const std::string& roomNumber,
                                               const std::string& startDate,
                                               const std::string& endDate,
                                               const std::string& note,
                                               std::string& errorMessage)
{
    const QDate startDay = QDate::fromString(QString::fromStdString(startDate), Qt::ISODate);
    const QDate endDay = QDate::fromString(QString::fromStdString(endDate), Qt::ISODate);
    const QDateTime maintenanceStart(startDay, QTime(0, 0));
    const QDateTime maintenanceEnd(endDay, QTime(0, 0));
    std::vector<std::string> affectedBookingIds;
    for (const auto& booking : m_bookingManager.getBookings()) {
        if (!booking || booking->isCancelled() || booking->isDeleted() ||
            !booking->getRoom() || booking->getRoom()->getRoomNumber() != roomNumber ||
            getBookingState(*booking) == BookingState::COMPLETED) {
            continue;
        }

        QDateTime plannedStart = QDateTime::fromString(
            QString::fromStdString(booking->getPlannedCheckInAt()), Qt::ISODateWithMs);
        QDateTime plannedEnd = QDateTime::fromString(
            QString::fromStdString(booking->getPlannedCheckOutAt()), Qt::ISODateWithMs);
        if (!plannedStart.isValid()) {
            const QDate legacyStart = QDate::fromString(QString::fromStdString(booking->getCheckInDate()), Qt::ISODate);
            plannedStart = legacyStart.isValid() ? QDateTime(legacyStart, QTime(0, 0)) : QDateTime();
        }
        if (!plannedEnd.isValid()) {
            const QDate legacyEnd = QDate::fromString(QString::fromStdString(booking->getCheckOutDate()), Qt::ISODate);
            plannedEnd = legacyEnd.isValid() ? QDateTime(legacyEnd, QTime(0, 0)) : QDateTime();
        }
        // Modified: Compare Maintenance against canonical planned timestamps so a same-day hourly stay cannot be missed by legacy date fields.
        if (maintenanceStart.isValid() && maintenanceEnd.isValid() && plannedStart.isValid() && plannedEnd.isValid()
            && maintenanceStart < plannedEnd && plannedStart < maintenanceEnd) {
            // Modified: A maintenance case cannot begin over an in-house guest. Upcoming reservations may use the soft-hold workflow,
            // but an Active stay requires reception to resolve the physical occupancy before maintenance is even scheduled.
            if (getBookingState(*booking) == BookingState::ACTIVE) {
                errorMessage = "Cannot schedule maintenance because active booking " + booking->getBookingId()
                    + " occupies Room " + roomNumber + " during the selected period. Check out or relocate the guest first.";
                return false;
            }
            affectedBookingIds.push_back(booking->getBookingId());
        }
    }

    return m_roomManager.scheduleRoomMaintenance(roomNumber, startDate, endDate, note, affectedBookingIds, errorMessage);
}

std::vector<std::string> HotelManager::getMaintenanceImpactWarnings(const std::string& roomNumber,
                                                                     const std::string& startDate,
                                                                     const std::string& endDate) const
{
    std::vector<std::string> warnings;
    const QDateTime maintenanceStart(QDate::fromString(QString::fromStdString(startDate), Qt::ISODate), QTime(0, 0));
    const QDateTime maintenanceEnd(QDate::fromString(QString::fromStdString(endDate), Qt::ISODate), QTime(0, 0));
    if (!maintenanceStart.isValid() || !maintenanceEnd.isValid() || maintenanceEnd <= maintenanceStart) {
        return warnings;
    }

    for (const auto& booking : m_bookingManager.getBookings()) {
        if (!booking || booking->isCancelled() || booking->isDeleted() || !booking->getRoom()
            || booking->getRoom()->getRoomNumber() != roomNumber) {
            continue;
        }
        const BookingState state = getBookingState(*booking);
        if (state != BookingState::UPCOMING && state != BookingState::ACTIVE) {
            continue;
        }
        const QDateTime plannedStart = plannedBookingBoundary(*booking, true);
        const QDateTime plannedEnd = plannedBookingBoundary(*booking, false);
        if (!plannedStart.isValid() || !plannedEnd.isValid()
            || !(maintenanceStart < plannedEnd && plannedStart < maintenanceEnd)) {
            continue;
        }
        const QString guest = booking->getCustomer()
            ? QString::fromStdString(booking->getCustomer()->getName()) : QStringLiteral("Guest unavailable");
        // Modified: Return a staff-readable impact list before maintenance becomes a soft hold or confirmed physical block.
        warnings.push_back(QString("%1 — %2 (%3), planned %4 to %5")
            .arg(QString::fromStdString(booking->getBookingId()), guest,
                 QString::fromStdString(bookingStateToString(state)), plannedStart.toString("dd MMM yyyy HH:mm"),
                 plannedEnd.toString("dd MMM yyyy HH:mm")).toStdString());
    }
    return warnings;
}

bool HotelManager::confirmRoomMaintenance(const std::string& maintenanceId, std::string& errorMessage)
{
    const auto maintenanceIt = std::find_if(m_roomManager.getRoomMaintenances().cbegin(), m_roomManager.getRoomMaintenances().cend(),
        [&maintenanceId](const RoomMaintenance& item) {
            return item.getMaintenanceId() == maintenanceId;
        });
    if (maintenanceIt == m_roomManager.getRoomMaintenances().cend()) {
        errorMessage = "Maintenance case not found.";
        return false;
    }
    if (maintenanceIt->isConfirmed()) {
        errorMessage = "Maintenance is already confirmed.";
        return false;
    }

    for (const auto& booking : m_bookingManager.getBookings()) {
        if (!booking || booking->isCancelled() || booking->isDeleted() || !booking->getRoom()
            || booking->getRoom()->getRoomNumber() != maintenanceIt->getRoomNumber()
            || getBookingState(*booking) == BookingState::COMPLETED) {
            continue;
        }
        const QDateTime maintenanceStart(QDate::fromString(QString::fromStdString(maintenanceIt->getStartDate()), Qt::ISODate), QTime(0, 0));
        const QDateTime maintenanceEnd(QDate::fromString(QString::fromStdString(maintenanceIt->getEndDate()), Qt::ISODate), QTime(0, 0));
        const QDateTime plannedStart = plannedBookingBoundary(*booking, true);
        const QDateTime plannedEnd = plannedBookingBoundary(*booking, false);
        // Modified: Recheck Maintenance confirmation against planned timestamps so hourly bookings cannot bypass the guest-notice workflow.
        if (maintenanceStart.isValid() && maintenanceEnd.isValid() && plannedStart.isValid() && plannedEnd.isValid()
            && maintenanceStart < plannedEnd && plannedStart < maintenanceEnd) {
            errorMessage = "Booking " + booking->getBookingId()
                + " still overlaps this maintenance case. Resolve the booking before confirmation.";
            return false;
        }
    }

    return m_roomManager.confirmRoomMaintenance(maintenanceId, errorMessage);
}

bool HotelManager::cancelRoomMaintenance(const std::string& maintenanceId, std::string& errorMessage)
{
    return m_roomManager.cancelRoomMaintenance(maintenanceId, errorMessage);
}

bool HotelManager::archiveRoom(const std::string& roomNumber, std::string& errorMessage)
{
    auto room = findRoomByNumber(roomNumber);
    if (!room)
    {
        errorMessage = "Room not found.";
        return false;
    }

    for (const auto &booking : m_bookingManager.getBookings())
    {
        if (!booking || booking->isCancelled() || booking->isDeleted())
            continue;

        auto bookedRoom = booking->getRoom();
        if (!bookedRoom || bookedRoom->getRoomNumber() != roomNumber)
            continue;

        const BookingState state = getBookingState(*booking);
        if (state == BookingState::UPCOMING || state == BookingState::ACTIVE)
        {
            errorMessage = "Cannot archive room while it has an active or upcoming booking.";
            return false;
        }
    }

    return m_roomManager.archiveRoom(roomNumber, errorMessage);
}

bool HotelManager::restoreRoom(const std::string& roomNumber, std::string& errorMessage)
{
    return m_roomManager.restoreRoom(roomNumber, errorMessage);
}

bool HotelManager::archiveCustomer(const std::string& customerId, std::string& errorMessage)
{
    auto customer = findCustomerById(customerId);
    if (!customer)
    {
        errorMessage = "Customer not found.";
        return false;
    }

    for (const auto &booking : m_bookingManager.getBookings())
    {
        if (!booking || booking->isCancelled() || booking->isDeleted())
            continue;

        auto bookingCustomer = booking->getCustomer();
        if (!bookingCustomer || bookingCustomer->getCustomerId() != customerId)
            continue;

        const BookingState state = getBookingState(*booking);
        if (state == BookingState::UPCOMING || state == BookingState::ACTIVE)
        {
            errorMessage = "Cannot archive customer while they have an active or upcoming booking.";
            return false;
        }
    }

    return m_customerManager.archiveCustomer(customerId, errorMessage);
}

bool HotelManager::restoreCustomer(const std::string& customerId, std::string& errorMessage)
{
    return m_customerManager.restoreCustomer(customerId, errorMessage);
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
    bool checkedIn,
    bool checkedOut,
    const std::string& actualCheckInDate,
    const std::string& actualCheckOutDate,
    double quotedUnitPrice,
    double quotedTaxRate,
    int adultCount,
    int childCount,
    const std::string& cancellationReason,
    const std::string& cancelledAt,
    const std::string& createdAt,
    const std::string& updatedAt,
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

    const auto customer = findCustomerById(customerId);
    if (!customer) {
        errorMessage = "Booking references missing customer: " + customerId;
        return false;
    }

    const auto room = findRoomByNumber(roomNumber);
    if (!room) {
        errorMessage = "Booking references missing room: " + roomNumber;
        return false;
    }
    if (checkedIn && !isValidDateString(actualCheckInDate, errorMessage)) {
        return false;
    }
    if (checkedOut) {
        if (!checkedIn || !isValidDateString(actualCheckOutDate, errorMessage)) {
            errorMessage = "Persisted completed booking is missing actual stay facts.";
            return false;
        }
        const QDate actualIn = QDate::fromString(QString::fromStdString(actualCheckInDate), Qt::ISODate);
        const QDate actualOut = QDate::fromString(QString::fromStdString(actualCheckOutDate), Qt::ISODate);
        // Modified: Restore same-day completed stays as one-night bookings, matching checkout and invoice validation.
        if (actualOut < actualIn) {
            errorMessage = "Persisted completed booking has an invalid actual stay duration.";
            return false;
        }
    }

    return m_bookingManager.restoreBookingFromDatabase(bookingId, customer, room, checkInDate, checkOutDate,
                                                      cancelled, deleted, checkedIn, checkedOut,
                                                      actualCheckInDate, actualCheckOutDate,
                                                      quotedUnitPrice, quotedTaxRate,
                                                      adultCount, childCount,
                                                      cancellationReason, cancelledAt,
                                                      createdAt, updatedAt, errorMessage);
}

bool HotelManager::restoreCustomerFromDatabase(
    const std::string &customerId,
    const std::string &documentType,
    const std::string &issuingCountry,
    const std::string &documentNumber,
    const std::string &name,
    const std::string &phone,
    bool archived,
    std::string &errorMessage)
{
    return m_customerManager.restoreCustomerFromDatabase(customerId, documentType, issuingCountry, documentNumber, name, phone, archived, errorMessage);
}

bool HotelManager::restoreInvoiceFromDatabase(
    const std::string &invoiceId,
    const std::string &bookingId,
    double taxRate,
    int nights,
    const std::string &invoiceIssuedDate,
    const std::string& paymentMethod,
    double paymentAmount,
    const std::string& paymentReceivedDate,
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
    return m_bookingManager.restoreInvoiceFromDatabase(
        invoiceId, bookingId, taxRate, nights, invoiceIssuedDate,
        paymentMethod, paymentAmount, paymentReceivedDate, unitPrice,
        customerNameSnapshot, customerIdSnapshot, customerPhoneSnapshot,
        roomNumberSnapshot, roomTypeSnapshot, checkInDateSnapshot,
        checkOutDateSnapshot, errorMessage);
}

bool HotelManager::restoreRoomMaintenanceFromDatabase(const std::string& maintenanceId,
                                                      const std::string& roomNumber,
                                                      const std::string& startDate,
                                                      const std::string& endDate,
                                                      const std::string& note,
                                                      const std::string& status,
                                                      const std::string& createdAt,
                                                      const std::string& blockType,
                                                      const std::string& startAt,
                                                      const std::string& endAt,
                                                      const std::string& completedAt,
                                                      const std::string& completedBy,
                                                      std::string& errorMessage)
{
    if (!m_roomManager.validateRoomMaintenanceRestoration(
            maintenanceId, roomNumber, startDate, endDate, status, blockType, startAt, endAt, errorMessage)) {
        return false;
    }
    if (blockType == "Maintenance" && status == "Confirmed"
        && hasRoomMaintenanceConflict(roomNumber, startDate, endDate, errorMessage)) {
        return false;
    }

    if (blockType == "Maintenance" && status == "Confirmed") {
        const QDateTime maintenanceStart(QDate::fromString(QString::fromStdString(startDate), Qt::ISODate), QTime(0, 0));
        const QDateTime maintenanceEnd(QDate::fromString(QString::fromStdString(endDate), Qt::ISODate), QTime(0, 0));
        for (const auto& booking : m_bookingManager.getBookings()) {
            if (!booking || booking->isCancelled() || booking->isDeleted()
                || !booking->getRoom() || booking->getRoom()->getRoomNumber() != roomNumber
                || getBookingState(*booking) == BookingState::COMPLETED) {
                continue;
            }
            const QDateTime plannedStart = plannedBookingBoundary(*booking, true);
            const QDateTime plannedEnd = plannedBookingBoundary(*booking, false);
            // Modified: Reject restored Maintenance that overlaps an unfinished hourly booking, including a same-day stay.
            if (maintenanceStart.isValid() && maintenanceEnd.isValid() && plannedStart.isValid() && plannedEnd.isValid()
                && maintenanceStart < plannedEnd && plannedStart < maintenanceEnd) {
                errorMessage = "Persisted maintenance conflicts with unfinished booking "
                    + booking->getBookingId() + ".";
                return false;
            }
        }
    }

    return m_roomManager.restoreRoomMaintenanceFromDatabase(
        maintenanceId, roomNumber, startDate, endDate, note, status, createdAt,
        blockType, startAt, endAt, completedAt, completedBy, errorMessage);
}

bool HotelManager::restoreMaintenanceGuestNoticeFromDatabase(const std::string& noticeId,
                                                             const std::string& maintenanceId,
                                                             const std::string& bookingId,
                                                             const std::string& channel,
                                                             const std::string& status,
                                                             const std::string& loggedAt,
                                                             std::string& errorMessage)
{
    if (!findBookingById(bookingId)) {
        errorMessage = "Persisted maintenance notification has a missing maintenance case or booking.";
        return false;
    }
    return m_roomManager.restoreMaintenanceGuestNoticeFromDatabase(noticeId, maintenanceId, bookingId, channel, status, loggedAt, errorMessage);
}

bool HotelManager::cancelBooking(const std::string &bookingId, const std::string& reason, std::string &errorMessage)
{
    return m_bookingManager.cancelBooking(bookingId, reason, errorMessage);
}

bool HotelManager::markNoShow(const std::string& bookingId, const std::string& reason, std::string& errorMessage)
{
    return m_bookingManager.markNoShow(bookingId, reason, errorMessage);
}

// =========================
// Delete methods
// =========================
// Modified: Allow room/customer deletion only without history and prevent destructive deletion of bookings or invoices.
bool HotelManager::deleteRoom(const std::string& roomNumber, std::string& errorMessage)
{
    auto room = findRoomByNumber(roomNumber);
    if (!room)
    {
        errorMessage = "Room not found.";
        return false;
    }

    for (const auto &booking : m_bookingManager.getBookings())
    {
        if (!booking || !booking->getRoom())
            continue;

        if (booking->getRoom()->getRoomNumber() == roomNumber)
        {
            errorMessage = "Cannot delete room because it is referenced by booking history.";
            return false;
        }
    }

    return m_roomManager.deleteRoom(roomNumber, errorMessage);
}

bool HotelManager::deleteCustomer(const std::string& customerId, std::string& errorMessage)
{
    auto customer = findCustomerById(customerId);
    if (!customer)
    {
        errorMessage = "Customer not found.";
        return false;
    }

    for (const auto &booking : m_bookingManager.getBookings())
    {
        if (!booking || !booking->getCustomer())
            continue;

        if (booking->getCustomer()->getCustomerId() == customerId)
        {
            errorMessage = "Cannot delete customer because they are referenced by booking history.";
            return false;
        }
    }

    return m_customerManager.deleteCustomer(customerId, errorMessage);
}

bool HotelManager::soft_deleteBooking(const std::string &bookingId, std::string &errorMessage)
{
    auto booking = findBookingById(bookingId);
    if (!booking)
    {
        errorMessage = "Booking not found.";
        return false;
    }

    // Modified: Preserve every reservation as operational history; staff must cancel rather than hide a booking.
    errorMessage = "Booking records cannot be deleted. Cancel the reservation to preserve audit history.";
    return false;
}

bool HotelManager::deleteInvoice(const std::string &invoiceId, std::string &errorMessage)
{
    auto invoice = findInvoiceById(invoiceId);
    if (!invoice)
    {
        errorMessage = "Invoice not found.";
        return false;
    }

    // Modified: Financial documents are immutable; a future credit-note workflow must reverse them instead of deleting them.
    errorMessage = "Invoices cannot be deleted because financial history must remain auditable.";
    return false;
}
