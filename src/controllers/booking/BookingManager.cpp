#include "BookingManager.h"

#include "Customer.h"
#include "Room.h"

#include <QDate>
#include <QDateTime>
#include <QTime>
#include <QString>

#include <algorithm>

namespace {
std::string bookingStateToStringLocal(BookingState state)
{
    switch (state) {
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

QDateTime parseIsoDateTime(const std::string& value)
{
    QDateTime parsed = QDateTime::fromString(QString::fromStdString(value), Qt::ISODateWithMs);
    if (!parsed.isValid()) {
        parsed = QDateTime::fromString(QString::fromStdString(value), Qt::ISODate);
    }
    return parsed;
}

QDateTime midnightForDate(const std::string& date)
{
    const QDate parsed = QDate::fromString(QString::fromStdString(date), Qt::ISODate);
    return parsed.isValid() ? QDateTime(parsed, QTime(0, 0)) : QDateTime();
}

QDateTime plannedStart(const Booking& booking)
{
    const QDateTime timestamp = parseIsoDateTime(booking.getPlannedCheckInAt());
    return timestamp.isValid() ? timestamp : midnightForDate(booking.getCheckInDate());
}

QDateTime plannedEnd(const Booking& booking)
{
    const QDateTime timestamp = parseIsoDateTime(booking.getPlannedCheckOutAt());
    return timestamp.isValid() ? timestamp : midnightForDate(booking.getCheckOutDate());
}

QDateTime roomBlockStart(const RoomMaintenance& block)
{
    const QDateTime timestamp = parseIsoDateTime(block.getStartAt());
    return timestamp.isValid() ? timestamp : midnightForDate(block.getStartDate());
}

QDateTime roomBlockEnd(const RoomMaintenance& block)
{
    QDateTime end = parseIsoDateTime(block.getEndAt());
    if (!end.isValid()) {
        end = midnightForDate(block.getEndDate());
    }
    const QDateTime completed = parseIsoDateTime(block.getCompletedAt());
    return completed.isValid() && completed < end ? completed : end;
}

bool overlaps(const QDateTime& firstStart, const QDateTime& firstEnd,
              const QDateTime& secondStart, const QDateTime& secondEnd)
{
    return firstStart < secondEnd && secondStart < firstEnd;
}

bool blocksPlannedAvailability(const RoomMaintenance& block)
{
    // Modified: Keep an awaiting Maintenance request as a soft hold so new reservations cannot be sold into a period awaiting guest relocation.
    return block.isConfirmed()
        || (block.isMaintenance() && block.getStatus() == "Awaiting guest response");
}
}

const std::vector<std::shared_ptr<Booking>>& BookingManager::getBookings() const
{
    return m_bookings;
}

std::shared_ptr<Booking> BookingManager::findBookingById(const std::string& bookingId) const
{
    for (const auto& booking : m_bookings) {
        if (booking && booking->getBookingId() == bookingId) {
            return booking;
        }
    }
    return nullptr;
}

bool BookingManager::bookingIdExists(const std::string& bookingId) const
{
    return findBookingById(bookingId) != nullptr;
}

BookingState BookingManager::getBookingState(const Booking& booking) const
{
    if (booking.isCancelled()) {
        return BookingState::CANCELLED;
    }

    if (booking.isCheckedOut()) {
        return BookingState::COMPLETED;
    }

    return booking.isCheckedIn() ? BookingState::ACTIVE : BookingState::UPCOMING;
}

std::vector<std::shared_ptr<Booking>> BookingManager::getBookingsForCustomer(const std::string& customerId) const
{
    std::vector<std::shared_ptr<Booking>> customerBookings;
    for (const auto& booking : m_bookings) {
        if (!booking) {
            continue;
        }
        const auto customer = booking->getCustomer();
        if (customer && customer->getCustomerId() == customerId) {
            customerBookings.push_back(booking);
        }
    }
    return customerBookings;
}

std::vector<std::shared_ptr<Booking>> BookingManager::getBookingsByStatus(BookingState state) const
{
    std::vector<std::shared_ptr<Booking>> filteredBookings;
    for (const auto& booking : m_bookings) {
        if (booking && !booking->isDeleted() && getBookingState(*booking) == state) {
            filteredBookings.push_back(booking);
        }
    }
    return filteredBookings;
}

bool BookingManager::hasBookingForRoom(const std::string& roomNumber) const
{
    for (const auto& booking : m_bookings) {
        if (!booking || !booking->getRoom()) {
            continue;
        }
        if (booking->getRoom()->getRoomNumber() == roomNumber) {
            return true;
        }
    }
    return false;
}

bool BookingManager::hasBookingForCustomer(const std::string& customerId) const
{
    for (const auto& booking : m_bookings) {
        if (!booking || !booking->getCustomer()) {
            continue;
        }
        if (booking->getCustomer()->getCustomerId() == customerId) {
            return true;
        }
    }
    return false;
}

bool BookingManager::validateBookingDates(const std::string& checkInDate,
                                          const std::string& checkOutDate,
                                          std::string& errorMessage) const
{
    const QDate checkIn = QDate::fromString(QString::fromStdString(checkInDate), Qt::ISODate);
    const QDate checkOut = QDate::fromString(QString::fromStdString(checkOutDate), Qt::ISODate);
    if (!checkIn.isValid() || !checkOut.isValid()) {
        errorMessage = "Dates must use ISO format (YYYY-MM-DD).";
        return false;
    }
    if (checkOut <= checkIn) {
        errorMessage = "Check-out must be after check-in.";
        return false;
    }
    return true;
}

bool BookingManager::isRoomFreeForDates(const std::string& roomNumber,
                                        const std::string& checkInDate,
                                        const std::string& checkOutDate,
                                        const std::vector<RoomMaintenance>& roomMaintenances,
                                        std::string& errorMessage,
                                        const std::string& excludedBookingId) const
{
    const QDate requestedCheckIn = QDate::fromString(QString::fromStdString(checkInDate), Qt::ISODate);
    const QDate requestedCheckOut = QDate::fromString(QString::fromStdString(checkOutDate), Qt::ISODate);
    if (!requestedCheckIn.isValid() || !requestedCheckOut.isValid() || requestedCheckOut <= requestedCheckIn) {
        errorMessage = "Dates must use a valid ISO date range.";
        return false;
    }
    return isRoomFreeForPeriod(roomNumber, checkInDate + "T00:00:00", checkOutDate + "T00:00:00",
                               roomMaintenances, errorMessage, excludedBookingId);
}

bool BookingManager::isRoomFreeForPeriod(const std::string& roomNumber,
                                         const std::string& plannedCheckInAt,
                                         const std::string& plannedCheckOutAt,
                                         const std::vector<RoomMaintenance>& roomMaintenances,
                                         std::string& errorMessage,
                                         const std::string& excludedBookingId) const
{
    const QDateTime requestedStart = parseIsoDateTime(plannedCheckInAt);
    const QDateTime requestedEnd = parseIsoDateTime(plannedCheckOutAt);
    if (!requestedStart.isValid() || !requestedEnd.isValid() || requestedEnd <= requestedStart) {
        errorMessage = "Planned check-in and check-out must form a valid time interval.";
        return false;
    }

    for (const RoomMaintenance& block : roomMaintenances) {
        if (!blocksPlannedAvailability(block) || block.getRoomNumber() != roomNumber) {
            continue;
        }
        const QDateTime blockStart = roomBlockStart(block);
        const QDateTime blockEnd = roomBlockEnd(block);
        if (blockStart.isValid() && blockEnd.isValid() && overlaps(requestedStart, requestedEnd, blockStart, blockEnd)) {
            const std::string availabilityReason = block.isConfirmed()
                ? block.getBlockType()
                : "a pending maintenance hold";
            errorMessage = "Room " + roomNumber + " is blocked for " + availabilityReason + " from "
                + blockStart.toString(Qt::ISODate).toStdString() + " to " + blockEnd.toString(Qt::ISODate).toStdString() + ".";
            return false;
        }
    }

    for (const auto& booking : m_bookings) {
        if (!booking || booking->isCancelled() || booking->isDeleted()
            || getBookingState(*booking) == BookingState::COMPLETED
            || (!excludedBookingId.empty() && booking->getBookingId() == excludedBookingId)) {
            continue;
        }
        const auto bookedRoom = booking->getRoom();
        if (!bookedRoom || bookedRoom->getRoomNumber() != roomNumber) {
            continue;
        }
        const QDateTime existingStart = plannedStart(*booking);
        const QDateTime existingEnd = plannedEnd(*booking);
        if (!existingStart.isValid() || !existingEnd.isValid()) {
            errorMessage = "Room " + roomNumber + " has a booking with an invalid planned schedule.";
            return false;
        }
        // Modified: Enforce the fixed two-hour turnover buffer on both sides of adjacent planned stays.
        if (overlaps(requestedStart, requestedEnd.addSecs(2 * 60 * 60),
                     existingStart, existingEnd.addSecs(2 * 60 * 60))) {
            // Modified: Explain the affected booking and earliest valid arrival so a rejected interval gives staff an actionable operational reason.
            errorMessage = "Room " + roomNumber + " needs a two-hour turnover buffer after booking "
                + booking->getBookingId() + " (planned checkout "
                + existingEnd.toString(Qt::ISODate).toStdString() + "). The next planned check-in may start from "
                + existingEnd.addSecs(2 * 60 * 60).toString(Qt::ISODate).toStdString() + ".";
            return false;
        }
    }
    return true;
}

const std::vector<std::shared_ptr<Invoice>>& BookingManager::getInvoices() const
{
    return m_invoices;
}

std::shared_ptr<Invoice> BookingManager::findInvoiceById(const std::string& invoiceId) const
{
    for (const auto& invoice : m_invoices) {
        if (invoice && invoice->getInvoiceId() == invoiceId) {
            return invoice;
        }
    }
    return nullptr;
}

std::shared_ptr<Invoice> BookingManager::findInvoiceForBooking(const std::string& bookingId) const
{
    for (const auto& invoice : m_invoices) {
        if (invoice && invoice->getBooking() && invoice->getBooking()->getBookingId() == bookingId) {
            return invoice;
        }
    }
    return nullptr;
}

bool BookingManager::invoiceIdExists(const std::string& invoiceId) const
{
    return findInvoiceById(invoiceId) != nullptr;
}

std::string BookingManager::nextInvoiceId() const
{
    int maxId = 1000;
    for (const auto& invoice : m_invoices) {
        if (!invoice) {
            continue;
        }
        const std::string id = invoice->getInvoiceId();
        if (id.rfind("INV", 0) == 0 && id.size() > 3) {
            try {
                maxId = std::max(maxId, std::stoi(id.substr(3)));
            } catch (...) {
                continue;
            }
        }
    }
    return "INV" + std::to_string(maxId + 1);
}

int BookingManager::calculateBillableHours(long long actualDurationSeconds)
{
    // Modified: Round actual elapsed time half-up at the 30-minute boundary and preserve the one-hour minimum.
    // This must remain the sole billing-duration rule until a future rate-plan engine introduces a different policy.
    return std::max(1, static_cast<int>((actualDurationSeconds + 30 * 60) / (60 * 60)));
}

bool BookingManager::validateBookingInput(const std::shared_ptr<Customer>& customer,
                                          const std::shared_ptr<Room>& room,
                                          const std::string& checkInDate,
                                          const std::string& checkOutDate,
                                          int adultCount,
                                          int childCount,
                                          const std::vector<RoomMaintenance>& roomMaintenances,
                                          std::string& errorMessage) const
{
    if (!validateBookingDates(checkInDate, checkOutDate, errorMessage)) {
        return false;
    }

    if (!customer) {
        errorMessage = "Customer not found.";
        return false;
    }
    if (customer->isArchived()) {
        errorMessage = "Cannot create booking for an archived customer.";
        return false;
    }

    if (!room) {
        errorMessage = "Room not found.";
        return false;
    }
    if (!room->getIsAvailable()) {
        errorMessage = "Room is permanently unavailable for booking.";
        return false;
    }
    if (room->isArchived()) {
        errorMessage = "Cannot create booking for an archived room.";
        return false;
    }
    if (adultCount <= 0 || childCount < 0 || adultCount + childCount > room->getMaximumGuests()) {
        errorMessage = "Guest count must include at least one adult and cannot exceed this room's capacity of "
            + std::to_string(room->getMaximumGuests()) + ".";
        return false;
    }

    return isRoomFreeForDates(room->getRoomNumber(), checkInDate, checkOutDate, roomMaintenances, errorMessage);
}

bool BookingManager::createBooking(const std::string& customerId,
                                   const std::string& roomNumber,
                                   const std::string& checkInDate,
                                   const std::string& checkOutDate,
                                   int adultCount,
                                   int childCount,
                                   std::string& errorMessage)
{
    errorMessage = "BookingManager requires canonical customer and room objects.";
    return false;
}

bool BookingManager::createBookingResolved(const std::shared_ptr<Customer>& customer,
                                           const std::shared_ptr<Room>& room,
                                           const std::string& checkInDate,
                                           const std::string& checkOutDate,
                                           int adultCount,
                                           int childCount,
                                           const std::vector<RoomMaintenance>& roomMaintenances,
                                           std::string& errorMessage)
{
    if (!validateBookingInput(customer, room, checkInDate, checkOutDate, adultCount, childCount, roomMaintenances, errorMessage)) {
        return false;
    }

    const QDate checkIn = QDate::fromString(QString::fromStdString(checkInDate), Qt::ISODate);
    if (checkIn < QDate::currentDate()) {
        errorMessage = "New bookings cannot use a past check-in date.";
        return false;
    }

    auto booking = std::make_shared<Booking>();
    booking->setBookingId(Booking::nextBookingId());
    booking->setCustomer(customer);
    booking->setRoom(room);
    booking->setCheckInDate(checkInDate);
    booking->setCheckOutDate(checkOutDate);
    // Modified: Keep legacy date fields for current screens while persisting the canonical planned time interval for Phase 3.
    booking->setPlannedCheckInAt(checkInDate + "T00:00:00");
    booking->setPlannedCheckOutAt(checkOutDate + "T00:00:00");
    booking->setLegacyDateOnlySchedule(true);
    booking->setCancelled(false);
    booking->setDeleted(false);
    booking->setCheckedIn(false);
    booking->setCheckedOut(false);
    booking->setAdultCount(adultCount);
    booking->setChildCount(childCount);
    const std::string createdTimestamp = QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toStdString();
    booking->setCreatedAt(createdTimestamp);
    booking->setUpdatedAt(createdTimestamp);
    // Modified: Lock the room's hourly base rate only; optional services will be represented as explicit future line items rather than implicit room-type surcharges.
    booking->setQuotedUnitPrice(room->getBasePrice());
    booking->setQuotedHourlyRate(room->getBasePrice());
    booking->setQuotedTaxRate(0.10);
    m_bookings.push_back(booking);
    return true;
}

bool BookingManager::updateBooking(const std::string& bookingId,
                                   const std::string& customerId,
                                   const std::string& roomNumber,
                                   const std::string& checkInDate,
                                   const std::string& checkOutDate,
                                   int adultCount,
                                   int childCount,
                                   std::string& errorMessage)
{
    errorMessage = "BookingManager requires canonical customer and room objects.";
    return false;
}

bool BookingManager::createBookingAtResolved(const std::shared_ptr<Customer>& customer,
                                             const std::shared_ptr<Room>& room,
                                             const std::string& plannedCheckInAt,
                                             const std::string& plannedCheckOutAt,
                                             int adultCount,
                                             int childCount,
                                             const std::vector<RoomMaintenance>& roomMaintenances,
                                             std::string& errorMessage)
{
    const QDateTime plannedIn = parseIsoDateTime(plannedCheckInAt);
    const QDateTime plannedOut = parseIsoDateTime(plannedCheckOutAt);
    if (!plannedIn.isValid() || !plannedOut.isValid() || plannedOut < plannedIn.addSecs(60 * 60)) {
        errorMessage = "A planned stay must be at least one hour and use valid ISO timestamps.";
        return false;
    }
    if (plannedIn < QDateTime::currentDateTime()) {
        errorMessage = "New bookings cannot start in the past.";
        return false;
    }
    if (!customer || customer->isArchived()) {
        errorMessage = "Customer is unavailable for booking.";
        return false;
    }
    if (!room || !room->getIsAvailable() || room->isArchived()) {
        errorMessage = "Room is unavailable for booking.";
        return false;
    }
    if (adultCount <= 0 || childCount < 0 || adultCount + childCount > room->getMaximumGuests()) {
        errorMessage = "Guest count must include at least one adult and fit the room capacity.";
        return false;
    }
    if (!isRoomFreeForPeriod(room->getRoomNumber(), plannedCheckInAt, plannedCheckOutAt,
                             roomMaintenances, errorMessage)) {
        return false;
    }

    auto booking = std::make_shared<Booking>();
    booking->setBookingId(Booking::nextBookingId());
    booking->setCustomer(customer);
    booking->setRoom(room);
    booking->setCheckInDate(plannedIn.date().toString(Qt::ISODate).toStdString());
    booking->setCheckOutDate(plannedOut.date().toString(Qt::ISODate).toStdString());
    booking->setPlannedCheckInAt(plannedIn.toString(Qt::ISODateWithMs).toStdString());
    booking->setPlannedCheckOutAt(plannedOut.toString(Qt::ISODateWithMs).toStdString());
    booking->setLegacyDateOnlySchedule(false);
    booking->setCancelled(false);
    booking->setDeleted(false);
    booking->setCheckedIn(false);
    booking->setCheckedOut(false);
    booking->setAdultCount(adultCount);
    booking->setChildCount(childCount);
    // Modified: Snapshot the selected room's hourly base rate; extra services are deliberately not inferred from room type.
    booking->setQuotedUnitPrice(room->getBasePrice());
    booking->setQuotedHourlyRate(room->getBasePrice());
    booking->setQuotedTaxRate(0.10);
    const std::string timestamp = QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toStdString();
    booking->setCreatedAt(timestamp);
    booking->setUpdatedAt(timestamp);
    // Modified: Timestamp reservations become the canonical schedule while legacy date fields remain report-compatible.
    m_bookings.push_back(booking);
    return true;
}

std::vector<std::shared_ptr<Room>> BookingManager::getAvailableRoomsForPeriod(
    const std::string& plannedCheckInAt,
    const std::string& plannedCheckOutAt,
    const std::vector<std::shared_ptr<Room>>& rooms,
    const std::vector<RoomMaintenance>& roomMaintenances,
    std::string& errorMessage,
    const std::string& excludedBookingId) const
{
    const QDateTime start = parseIsoDateTime(plannedCheckInAt);
    const QDateTime end = parseIsoDateTime(plannedCheckOutAt);
    if (!start.isValid() || !end.isValid() || end < start.addSecs(60 * 60)) {
        errorMessage = "A room search requires a valid planned interval of at least one hour.";
        return {};
    }
    std::vector<std::shared_ptr<Room>> availableRooms;
    availableRooms.reserve(rooms.size());
    for (const auto& room : rooms) {
        if (!room || !room->getIsAvailable() || room->isArchived()) {
            continue;
        }
        std::string roomError;
        if (isRoomFreeForPeriod(room->getRoomNumber(), plannedCheckInAt, plannedCheckOutAt,
                                roomMaintenances, roomError, excludedBookingId)) {
            availableRooms.push_back(room);
        }
    }
    return availableRooms;
}

bool BookingManager::updateBookingResolved(const std::string& bookingId,
                                           const std::string& customerId,
                                           const std::string& roomNumber,
                                           const std::shared_ptr<Customer>& customer,
                                           const std::shared_ptr<Room>& room,
                                           const std::string& checkInDate,
                                           const std::string& checkOutDate,
                                           int adultCount,
                                           int childCount,
                                           const std::vector<RoomMaintenance>& roomMaintenances,
                                           std::string& errorMessage)
{
    const auto booking = findBookingById(bookingId);
    if (!booking) {
        errorMessage = "Booking not found.";
        return false;
    }
    if (booking->isDeleted()) {
        errorMessage = "Cannot edit a deleted booking.";
        return false;
    }
    if (booking->isCancelled()) {
        errorMessage = "Cannot edit a cancelled booking.";
        return false;
    }

    const BookingState currentState = getBookingState(*booking);
    // Modified: Freeze every non-upcoming reservation so an active stay remains an auditable operational record until checkout.
    if (currentState != BookingState::UPCOMING) {
        errorMessage = "Only upcoming bookings can be edited. Active stays must be checked out instead.";
        return false;
    }
    if (!validateBookingDates(checkInDate, checkOutDate, errorMessage)) {
        return false;
    }

    const QDate proposedCheckIn = QDate::fromString(QString::fromStdString(checkInDate), Qt::ISODate);
    if (currentState == BookingState::UPCOMING && proposedCheckIn < QDate::currentDate()) {
        errorMessage = "Upcoming bookings cannot be moved to a past check-in date.";
        return false;
    }
    // Modified: Reject mismatched resolved entities so legacy ID-based callers cannot save a booking against a different in-memory customer or room.
    if (!customer || !room || customer->getCustomerId() != customerId || room->getRoomNumber() != roomNumber) {
        errorMessage = "The selected customer or room does not match the reservation details.";
        return false;
    }
    if (!validateBookingInput(customer, room, checkInDate, checkOutDate, adultCount, childCount, roomMaintenances, errorMessage)) {
        return false;
    }

    if (!isRoomFreeForDates(roomNumber, checkInDate, checkOutDate, roomMaintenances, errorMessage, bookingId)) {
        return false;
    }

    booking->setCustomer(customer);
    booking->setRoom(room);
    booking->setCheckInDate(checkInDate);
    booking->setCheckOutDate(checkOutDate);
    booking->setPlannedCheckInAt(checkInDate + "T00:00:00");
    booking->setPlannedCheckOutAt(checkOutDate + "T00:00:00");
    booking->setLegacyDateOnlySchedule(true);
    booking->setCancelled(false);
    booking->setAdultCount(adultCount);
    booking->setChildCount(childCount);
    if (currentState == BookingState::UPCOMING) {
        // Modified: Re-quote an upcoming reservation from the hourly room base rate, excluding future optional service line items.
        booking->setQuotedUnitPrice(room->getBasePrice());
        booking->setQuotedHourlyRate(room->getBasePrice());
    }
    booking->setUpdatedAt(QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toStdString());
    return true;
}

bool BookingManager::updateBookingAtResolved(const std::string& bookingId,
                                             const std::shared_ptr<Customer>& customer,
                                             const std::shared_ptr<Room>& room,
                                             const std::string& plannedCheckInAt,
                                             const std::string& plannedCheckOutAt,
                                             int adultCount,
                                             int childCount,
                                             const std::vector<RoomMaintenance>& roomMaintenances,
                                             std::string& errorMessage)
{
    const auto booking = findBookingById(bookingId);
    if (!booking || booking->isDeleted() || booking->isCancelled()) {
        errorMessage = "Only an existing upcoming booking can be edited.";
        return false;
    }
    const BookingState state = getBookingState(*booking);
    // Modified: Enforce the same immutable-active-stay rule for timestamp reservations and legacy date-only callers.
    if (state != BookingState::UPCOMING) {
        errorMessage = "Only upcoming bookings can be edited. Active stays must be checked out instead.";
        return false;
    }
    const QDateTime plannedIn = parseIsoDateTime(plannedCheckInAt);
    const QDateTime plannedOut = parseIsoDateTime(plannedCheckOutAt);
    if (!plannedIn.isValid() || !plannedOut.isValid() || plannedOut < plannedIn.addSecs(60 * 60)) {
        errorMessage = "A planned stay must be at least one hour and use valid ISO timestamps.";
        return false;
    }
    if (!customer || customer->isArchived() || !room || !room->getIsAvailable() || room->isArchived()) {
        errorMessage = "Customer or room is unavailable for this booking.";
        return false;
    }
    if (adultCount <= 0 || childCount < 0 || adultCount + childCount > room->getMaximumGuests()) {
        errorMessage = "Guest count must include at least one adult and fit the room capacity.";
        return false;
    }
    if (!isRoomFreeForPeriod(room->getRoomNumber(), plannedCheckInAt, plannedCheckOutAt,
                             roomMaintenances, errorMessage, bookingId)) {
        return false;
    }

    booking->setCustomer(customer);
    booking->setRoom(room);
    booking->setCheckInDate(plannedIn.date().toString(Qt::ISODate).toStdString());
    booking->setCheckOutDate(plannedOut.date().toString(Qt::ISODate).toStdString());
    booking->setPlannedCheckInAt(plannedIn.toString(Qt::ISODateWithMs).toStdString());
    booking->setPlannedCheckOutAt(plannedOut.toString(Qt::ISODateWithMs).toStdString());
    booking->setLegacyDateOnlySchedule(false);
    booking->setAdultCount(adultCount);
    booking->setChildCount(childCount);
    if (state == BookingState::UPCOMING) {
        // Modified: Preserve a base-rate-only hourly quote whenever an upcoming timestamp reservation changes rooms.
        booking->setQuotedUnitPrice(room->getBasePrice());
        booking->setQuotedHourlyRate(room->getBasePrice());
    }
    // Modified: Editing a timestamp schedule updates planned facts only; actual stay facts remain auditable evidence.
    booking->setUpdatedAt(QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toStdString());
    return true;
}

bool BookingManager::extendActiveBookingAt(const std::string& bookingId,
                                           const std::string& plannedCheckOutAt,
                                           const std::vector<RoomMaintenance>& roomMaintenances,
                                           std::string& errorMessage)
{
    const auto booking = findBookingById(bookingId);
    if (!booking || booking->isDeleted() || booking->isCancelled() || getBookingState(*booking) != BookingState::ACTIVE) {
        errorMessage = "Only an active stay can be extended.";
        return false;
    }
    const auto room = booking->getRoom();
    if (!room) {
        errorMessage = "The active stay has no assigned room.";
        return false;
    }

    const QDateTime currentPlannedEnd = plannedEnd(*booking);
    const QDateTime requestedEnd = parseIsoDateTime(plannedCheckOutAt);
    if (!currentPlannedEnd.isValid() || !requestedEnd.isValid() || requestedEnd <= currentPlannedEnd) {
        errorMessage = "The extended planned check-out must be later than the current planned check-out.";
        return false;
    }
    if (requestedEnd <= QDateTime::currentDateTime()) {
        errorMessage = "The extended planned check-out must be in the future.";
        return false;
    }
    if (!isRoomFreeForPeriod(room->getRoomNumber(), plannedStart(*booking).toString(Qt::ISODateWithMs).toStdString(),
                             plannedCheckOutAt, roomMaintenances, errorMessage, bookingId)) {
        return false;
    }

    // Modified: Allow an active guest to extend only the planned departure while preserving the original guest, room, actual check-in, and rate snapshot.
    booking->setCheckOutDate(requestedEnd.date().toString(Qt::ISODate).toStdString());
    booking->setPlannedCheckOutAt(requestedEnd.toString(Qt::ISODateWithMs).toStdString());
    booking->setLegacyDateOnlySchedule(false);
    booking->setUpdatedAt(QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toStdString());
    return true;
}

bool BookingManager::checkInBooking(const std::string& bookingId, const std::string& checkInDate,
                                   std::string& errorMessage)
{
    const QDate date = QDate::fromString(QString::fromStdString(checkInDate), Qt::ISODate);
    if (!date.isValid()) {
        errorMessage = "Check-in date must use ISO format (YYYY-MM-DD).";
        return false;
    }
    return checkInBookingAt(bookingId,
                            checkInDate + "T" + QTime::currentTime().toString("HH:mm:ss").toStdString(),
                            errorMessage);
}

bool BookingManager::checkInBookingAt(const std::string& bookingId, const std::string& actualCheckInAt,
                                      std::string& errorMessage)
{
    const auto booking = findBookingById(bookingId);
    if (!booking || booking->isDeleted() || booking->isCancelled()) {
        errorMessage = "Only an existing upcoming reservation can be checked in.";
        return false;
    }
    if (booking->isCheckedIn() || booking->isCheckedOut()) {
        errorMessage = "This reservation has already been checked in or checked out.";
        return false;
    }
    const QDateTime actualCheckIn = parseIsoDateTime(actualCheckInAt);
    const QDateTime plannedCheckIn = plannedStart(*booking);
    const QDateTime plannedCheckOut = plannedEnd(*booking);
    if (!actualCheckIn.isValid() || actualCheckIn.date() != QDate::currentDate()) {
        errorMessage = "Check-in must be recorded for today.";
        return false;
    }
    if (!plannedCheckIn.isValid() || !plannedCheckOut.isValid() || actualCheckIn >= plannedCheckOut) {
        errorMessage = "Check-in must be before the planned departure.";
        return false;
    }
    // Modified: Enforce the planned arrival day in the backend while still allowing a room-ready guest to check in earlier on that same day.
    if (actualCheckIn.date() < plannedCheckIn.date()) {
        errorMessage = "Check-in cannot be recorded before the planned arrival date.";
        return false;
    }
    booking->setCheckedIn(true);
    booking->setActualCheckInDate(actualCheckIn.date().toString(Qt::ISODate).toStdString());
    // Modified: Record the actual check-in moment separately so future hourly billing never has to infer a time from a date.
    booking->setActualCheckInAt(actualCheckIn.toString(Qt::ISODateWithMs).toStdString());
    booking->setUpdatedAt(QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toStdString());
    return true;
}

bool BookingManager::completeBooking(const std::string& bookingId,
                                     const std::string& checkoutDate,
                                     std::string& errorMessage)
{
    const QDate date = QDate::fromString(QString::fromStdString(checkoutDate), Qt::ISODate);
    if (!date.isValid()) {
        errorMessage = "Checkout date must use ISO format (YYYY-MM-DD).";
        return false;
    }
    return completeBookingAt(bookingId,
                             checkoutDate + "T" + QTime::currentTime().toString("HH:mm:ss").toStdString(),
                             errorMessage);
}

bool BookingManager::completeBookingAt(const std::string& bookingId,
                                       const std::string& actualCheckOutAt,
                                       std::string& errorMessage)
{
    const auto booking = findBookingById(bookingId);
    if (!booking) {
        errorMessage = "Booking not found.";
        return false;
    }
    if (booking->isDeleted()) {
        errorMessage = "Cannot complete a deleted booking.";
        return false;
    }
    if (booking->isCancelled()) {
        errorMessage = "Cannot complete a cancelled booking.";
        return false;
    }
    if (getBookingState(*booking) != BookingState::ACTIVE) {
        errorMessage = "Only active bookings can be checked out.";
        return false;
    }

    const QDateTime actualCheckout = parseIsoDateTime(actualCheckOutAt);
    QDateTime actualCheckIn = parseIsoDateTime(booking->getActualCheckInAt());
    if (!actualCheckIn.isValid()) {
        actualCheckIn = midnightForDate(booking->getActualCheckInDate());
    }
    if (!actualCheckout.isValid() || !actualCheckIn.isValid()) {
        errorMessage = "The actual check-in and check-out timestamps must use ISO format.";
        return false;
    }
    if (actualCheckout <= actualCheckIn) {
        errorMessage = "Actual checkout must be after actual check-in.";
        return false;
    }
    if (actualCheckout > QDateTime::currentDateTime()) {
        errorMessage = "Checkout cannot be recorded in the future.";
        return false;
    }

    booking->setActualCheckOutDate(actualCheckout.date().toString(Qt::ISODate).toStdString());
    // Modified: Record the actual checkout moment before the HotelManager creates its linked Cleaning block.
    booking->setActualCheckOutAt(actualCheckout.toString(Qt::ISODateWithMs).toStdString());
    booking->setCancelled(false);
    booking->setCheckedOut(true);
    booking->setUpdatedAt(QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toStdString());
    return true;
}

bool BookingManager::revertCompletedBooking(const std::string& bookingId, std::string& errorMessage)
{
    const auto booking = findBookingById(bookingId);
    if (!booking || getBookingState(*booking) != BookingState::COMPLETED) {
        errorMessage = "Only a newly completed booking can be restored to Active.";
        return false;
    }

    // Modified: Roll back checkout facts when its mandatory Cleaning block cannot be created, keeping room operations and booking state atomic in memory.
    booking->setCheckedOut(false);
    booking->setActualCheckOutDate("");
    booking->setActualCheckOutAt("");
    booking->setUpdatedAt(QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toStdString());
    return true;
}

bool BookingManager::cancelBooking(const std::string& bookingId, const std::string& reason, std::string& errorMessage)
{
    const auto booking = findBookingById(bookingId);
    if (!booking) {
        errorMessage = "Booking record not found.";
        return false;
    }
    if (booking->isDeleted()) {
        errorMessage = "This booking has already been deleted.";
        return false;
    }

    const BookingState state = getBookingState(*booking);
    if (state != BookingState::UPCOMING) {
        if (state == BookingState::ACTIVE) {
            errorMessage = "This booking is active. Only upcoming reservations can be cancelled.";
        } else if (state == BookingState::COMPLETED) {
            errorMessage = "This booking has already completed. Only upcoming reservations can be cancelled.";
        } else {
            errorMessage = "This booking has already been cancelled.";
        }
        return false;
    }

    booking->setCancelled(true);
    booking->setCancellationReason(reason.empty() ? "Cancelled by staff" : reason);
    booking->setCancelledAt(QDate::currentDate().toString(Qt::ISODate).toStdString());
    booking->setUpdatedAt(QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toStdString());
    return true;
}

bool BookingManager::markNoShow(const std::string& bookingId, const std::string& reason, std::string& errorMessage)
{
    // Modified: Preserve old callers temporarily but merge no-show into the single Upcoming cancellation workflow.
    return cancelBooking(bookingId, reason.empty() ? "Guest did not arrive" : reason, errorMessage);
}

bool BookingManager::createInvoice(const std::string& invoiceId,
                                   const std::string& bookingId,
                                   const std::string& invoiceIssuedDate,
                                   const std::string& paymentMethod,
                                   double paymentAmount,
                                   const std::string& paymentReceivedDate,
                                   std::string& errorMessage)
{
    if (invoiceId.empty()) {
        errorMessage = "Invoice ID is required.";
        return false;
    }
    if (invoiceIdExists(invoiceId)) {
        errorMessage = "Invoice ID already exists.";
        return false;
    }
    const QDate issuedDate = QDate::fromString(QString::fromStdString(invoiceIssuedDate), Qt::ISODate);
    if (!issuedDate.isValid()) {
        errorMessage = "Date must use ISO format (YYYY-MM-DD).";
        return false;
    }
    const QDate receivedDate = QDate::fromString(QString::fromStdString(paymentReceivedDate), Qt::ISODate);
    if (paymentMethod.empty() || paymentAmount <= 0.0 || !receivedDate.isValid() || receivedDate > QDate::currentDate()) {
        errorMessage = "A positive payment amount, method, and valid received date are required at checkout.";
        return false;
    }

    const auto booking = findBookingById(bookingId);
    if (!booking) {
        errorMessage = "Booking not found.";
        return false;
    }
    if (booking->isDeleted()) {
        errorMessage = "Cannot create invoice for a deleted booking.";
        return false;
    }
    if (getBookingState(*booking) != BookingState::COMPLETED) {
        errorMessage = "Invoice can only be created after checkout.";
        return false;
    }
    if (findInvoiceForBooking(bookingId)) {
        errorMessage = "An invoice already exists for this booking.";
        return false;
    }
    const QDate checkout = QDate::fromString(QString::fromStdString(booking->getActualCheckOutDate()), Qt::ISODate);
    if (!checkout.isValid() || issuedDate < checkout || issuedDate > QDate::currentDate()) {
        errorMessage = "Invoice issue date must be between the actual checkout date and today.";
        return false;
    }

    QDateTime actualCheckIn = parseIsoDateTime(booking->getActualCheckInAt());
    QDateTime actualCheckOut = parseIsoDateTime(booking->getActualCheckOutAt());
    if (!actualCheckIn.isValid()) {
        actualCheckIn = midnightForDate(booking->getActualCheckInDate());
    }
    if (!actualCheckOut.isValid()) {
        actualCheckOut = midnightForDate(booking->getActualCheckOutDate());
    }
    if (!actualCheckIn.isValid() || !actualCheckOut.isValid() || actualCheckOut <= actualCheckIn) {
        errorMessage = "The completed booking has an invalid actual stay duration.";
        return false;
    }
    const long long actualDurationSeconds = actualCheckIn.secsTo(actualCheckOut);
    const int billableHours = calculateBillableHours(actualDurationSeconds);

    auto invoice = std::make_shared<Invoice>();
    invoice->setInvoiceId(invoiceId);
    invoice->setBookingId(bookingId);
    invoice->captureBookingSnapshot(booking);
    invoice->setTaxRate(booking->getQuotedTaxRate());
    invoice->setNights(0);
    invoice->setActualDurationSeconds(actualDurationSeconds);
    invoice->setBillableHours(billableHours);
    invoice->setLegacyNightlyBilling(false);
    invoice->setHourlyRoomRateSnapshot(booking->getQuotedHourlyRate() > 0.0
        ? booking->getQuotedHourlyRate() : booking->getQuotedUnitPrice());
    invoice->setInvoiceIssuedDate(invoiceIssuedDate);
    invoice->setPaymentMethod(paymentMethod);
    invoice->setPaymentAmount(paymentAmount);
    invoice->setPaymentReceivedDate(paymentReceivedDate);
    if (paymentAmount > invoice->calculateTotal()) {
        errorMessage = "Payment amount cannot exceed the invoice total in the current single-payment workflow.";
        return false;
    }
    if (!invoice->isValid()) {
        errorMessage = "Failed to validate invoice details.";
        return false;
    }

    m_invoices.push_back(invoice);
    return true;
}

bool BookingManager::restoreBookingFromDatabase(const std::string& bookingId,
                                                const std::shared_ptr<Customer>& customer,
                                                const std::shared_ptr<Room>& room,
                                                const std::string& checkInDate,
                                                const std::string& checkOutDate,
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
                                                std::string& errorMessage)
{
    if (bookingId.empty()) {
        errorMessage = "Persisted booking ID is empty.";
        return false;
    }
    if (bookingIdExists(bookingId)) {
        errorMessage = "Duplicate persisted booking ID: " + bookingId;
        return false;
    }
    if (!customer) {
        errorMessage = "Booking references missing customer.";
        return false;
    }
    if (!room) {
        errorMessage = "Booking references missing room.";
        return false;
    }
    const QDate persistedCheckIn = QDate::fromString(QString::fromStdString(checkInDate), Qt::ISODate);
    const QDate persistedCheckOut = QDate::fromString(QString::fromStdString(checkOutDate), Qt::ISODate);
    // Modified: Allow equal legacy date fields during restoration because an hourly reservation can start and end on the same calendar date.
    // Timestamp hydration below remains authoritative for non-legacy bookings and still requires at least one planned hour.
    if (!persistedCheckIn.isValid() || !persistedCheckOut.isValid() || persistedCheckOut < persistedCheckIn) {
        errorMessage = "Persisted booking dates are invalid.";
        return false;
    }

    if (quotedUnitPrice <= 0.0 || quotedTaxRate < 0.0 || quotedTaxRate > 1.0 || adultCount <= 0
        || childCount < 0 || adultCount + childCount > room->getMaximumGuests()) {
        errorMessage = "Persisted booking pricing or guest occupancy is invalid.";
        return false;
    }
    if (checkedIn && actualCheckInDate.empty()) {
        errorMessage = "Persisted completed booking is missing actual stay facts.";
        return false;
    }
    if (checkedOut && actualCheckOutDate.empty()) {
        errorMessage = "Persisted completed booking is missing actual stay facts.";
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
    booking->setCheckedIn(checkedIn);
    booking->setCheckedOut(checkedOut);
    booking->setActualCheckInDate(actualCheckInDate);
    booking->setActualCheckOutDate(actualCheckOutDate);
    booking->setQuotedUnitPrice(quotedUnitPrice);
    booking->setQuotedTaxRate(quotedTaxRate);
    booking->setAdultCount(adultCount);
    booking->setChildCount(childCount);
    booking->setCancellationReason(cancellationReason);
    booking->setCancelledAt(cancelledAt);
    booking->setCreatedAt(createdAt);
    booking->setUpdatedAt(updatedAt);

    if (!booking->isValid()) {
        errorMessage = "Persisted booking is invalid.";
        return false;
    }

    m_bookings.push_back(booking);
    return true;
}

bool BookingManager::restoreInvoiceFromDatabase(
    const std::string& invoiceId,
    const std::string& bookingId,
    double taxRate,
    int nights,
    const std::string& invoiceIssuedDate,
    const std::string& paymentMethod,
    double paymentAmount,
    const std::string& paymentReceivedDate,
    double unitPrice,
    const std::string& customerNameSnapshot,
    const std::string& customerIdSnapshot,
    const std::string& customerPhoneSnapshot,
    const std::string& roomNumberSnapshot,
    const std::string& roomTypeSnapshot,
    const std::string& checkInDateSnapshot,
    const std::string& checkOutDateSnapshot,
    std::string& errorMessage)
{
    if (invoiceId.empty()) {
        errorMessage = "Persisted invoice ID is empty.";
        return false;
    }
    if (invoiceIdExists(invoiceId)) {
        errorMessage = "Duplicate persisted invoice ID: " + invoiceId;
        return false;
    }

    const auto booking = findBookingById(bookingId);
    if (!booking) {
        errorMessage = "Booking not found for persisted invoice.";
        return false;
    }
    if (booking->isDeleted()) {
        errorMessage = "Invoice can not be restored for a deleted booking.";
        return false;
    }
    if (taxRate < 0 || taxRate > 1) {
        errorMessage = "Tax rate must be between 0% and 100%.";
        return false;
    }
    if (nights < 0) {
        errorMessage = "Persisted stay duration cannot be negative.";
        return false;
    }

    const QDate issuedDate = QDate::fromString(QString::fromStdString(invoiceIssuedDate), Qt::ISODate);
    if (!issuedDate.isValid()) {
        errorMessage = invoiceIssuedDate.empty()
            ? "Date cannot be empty."
            : "Date must use ISO format (YYYY-MM-DD).";
        return false;
    }
    const QDate paymentDate = QDate::fromString(QString::fromStdString(paymentReceivedDate), Qt::ISODate);
    const QDate snapshotCheckIn = QDate::fromString(QString::fromStdString(checkInDateSnapshot), Qt::ISODate);
    const QDate snapshotCheckout = QDate::fromString(QString::fromStdString(checkOutDateSnapshot), Qt::ISODate);
    if (paymentMethod.empty() || paymentAmount <= 0 || !paymentDate.isValid()
        || unitPrice <= 0 || customerNameSnapshot.empty() || customerIdSnapshot.empty() || customerPhoneSnapshot.empty()
        || roomNumberSnapshot.empty() || roomTypeSnapshot.empty()
        || !snapshotCheckIn.isValid() || !snapshotCheckout.isValid()) {
        errorMessage = "Persisted invoice snapshot is incomplete or invalid.";
        return false;
    }
    if (issuedDate < snapshotCheckout || issuedDate > QDate::currentDate()) {
        errorMessage = "Persisted invoice issue date is outside the allowed range.";
        return false;
    }
    if (getBookingState(*booking) != BookingState::COMPLETED) {
        errorMessage = "Invoice can only be restored for a completed booking.";
        return false;
    }
    if (findInvoiceForBooking(bookingId)) {
        errorMessage = "An invoice already exists for this booking.";
        return false;
    }

    auto invoice = std::make_shared<Invoice>();
    invoice->setInvoiceId(invoiceId);
    invoice->setBookingId(bookingId);
    invoice->setBooking(booking);
    invoice->setTaxRate(taxRate);
    invoice->setNights(nights);
    invoice->setInvoiceIssuedDate(invoiceIssuedDate);
    invoice->setPaymentMethod(paymentMethod);
    invoice->setPaymentAmount(paymentAmount);
    invoice->setPaymentReceivedDate(paymentReceivedDate);
    invoice->setUnitPrice(unitPrice);
    invoice->setCustomerNameSnapshot(customerNameSnapshot);
    invoice->setCustomerIdSnapshot(customerIdSnapshot);
    invoice->setCustomerPhoneSnapshot(customerPhoneSnapshot);
    invoice->setRoomNumberSnapshot(roomNumberSnapshot);
    invoice->setRoomTypeSnapshot(roomTypeSnapshot);
    invoice->setCheckInDateSnapshot(checkInDateSnapshot);
    invoice->setCheckOutDateSnapshot(checkOutDateSnapshot);
    // Modified: DataManager attaches timestamp billing facts immediately after this legacy-compatible restore call,
    // then validates the complete object graph before publishing it.

    m_invoices.push_back(invoice);
    return true;
}

void BookingManager::clearAll()
{
    m_bookings.clear();
    m_invoices.clear();
}
