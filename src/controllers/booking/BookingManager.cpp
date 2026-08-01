#include "BookingManager.h"

#include "Customer.h"
#include "Room.h"

#include <QDate>
#include <QDateTime>
#include <QString>

#include <algorithm>
#include <unordered_set>

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
        return "No-show";
    default:
        return "Unknown";
    }
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
        if (booking.getCancellationReason().rfind("No-show:", 0) == 0) {
            return BookingState::NO_SHOW;
        }
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

std::vector<std::shared_ptr<Booking>> BookingManager::getArrivalsByDate(const std::string& dateStr) const
{
    std::vector<std::shared_ptr<Booking>> checkIns;
    for (const auto& booking : m_bookings) {
        if (booking && booking->getCheckInDate() == dateStr && !booking->isCancelled() && !booking->isDeleted()) {
            checkIns.push_back(booking);
        }
    }
    return checkIns;
}

std::vector<std::shared_ptr<Booking>> BookingManager::getDeparturesByDate(const std::string& dateStr) const
{
    std::vector<std::shared_ptr<Booking>> checkOuts;
    for (const auto& booking : m_bookings) {
        if (booking && booking->getCheckOutDate() == dateStr && !booking->isCancelled() && !booking->isDeleted()) {
            checkOuts.push_back(booking);
        }
    }
    return checkOuts;
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
    const QDate today = QDate::currentDate();

    for (const RoomMaintenance& maintenance : roomMaintenances) {
        if (maintenance.getRoomNumber() == roomNumber
            && checkInDate < maintenance.getEndDate() && maintenance.getStartDate() < checkOutDate) {
            errorMessage = "Room " + roomNumber + " has a " + maintenance.getStatus() + " maintenance case from "
                + maintenance.getStartDate() + " to " + maintenance.getEndDate() + ".";
            return false;
        }
    }

    for (const auto& booking : m_bookings) {
        if (!booking || booking->isCancelled() || booking->isDeleted()) {
            continue;
        }
        if (getBookingState(*booking) == BookingState::COMPLETED) {
            continue;
        }
        if (!excludedBookingId.empty() && booking->getBookingId() == excludedBookingId) {
            continue;
        }

        const auto bookedRoom = booking->getRoom();
        if (!bookedRoom || bookedRoom->getRoomNumber() != roomNumber) {
            continue;
        }

        if (getBookingState(*booking) == BookingState::ACTIVE
            && requestedCheckIn <= today && today < requestedCheckOut) {
            errorMessage = "Room " + roomNumber + " still has an active stay that must be checked out first.";
            return false;
        }

        const bool overlaps = checkInDate < booking->getCheckOutDate()
            && booking->getCheckInDate() < checkOutDate;
        if (overlaps) {
            errorMessage = "Room " + roomNumber + " is already booked from "
                + booking->getCheckInDate() + " to " + booking->getCheckOutDate()
                + " (" + bookingStateToStringLocal(getBookingState(*booking)) + ").";
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

std::vector<std::shared_ptr<Room>> BookingManager::getAvailableRoomsForDates(
    const std::string& checkInDate,
    const std::string& checkOutDate,
    const std::vector<std::shared_ptr<Room>>& rooms,
    const std::vector<RoomMaintenance>& roomMaintenances,
    std::string& errorMessage,
    const std::string& excludedBookingId) const
{
    const QDate requestedCheckIn = QDate::fromString(QString::fromStdString(checkInDate), Qt::ISODate);
    const QDate requestedCheckOut = QDate::fromString(QString::fromStdString(checkOutDate), Qt::ISODate);
    if (!requestedCheckIn.isValid() || !requestedCheckOut.isValid() || requestedCheckOut <= requestedCheckIn) {
        errorMessage = "Dates must use a valid ISO date range.";
        return {};
    }

    std::unordered_set<std::string> unavailableRoomNumbers;
    for (const RoomMaintenance& maintenance : roomMaintenances) {
        if (checkInDate < maintenance.getEndDate() && maintenance.getStartDate() < checkOutDate) {
            unavailableRoomNumbers.insert(maintenance.getRoomNumber());
        }
    }

    const QDate today = QDate::currentDate();
    for (const auto& booking : m_bookings) {
        if (!booking || booking->isCancelled() || booking->isDeleted()
            || (!excludedBookingId.empty() && booking->getBookingId() == excludedBookingId)) {
            continue;
        }
        if (getBookingState(*booking) == BookingState::COMPLETED) {
            continue;
        }

        const auto bookedRoom = booking->getRoom();
        if (!bookedRoom) {
            continue;
        }

        const bool overlaps = checkInDate < booking->getCheckOutDate()
            && booking->getCheckInDate() < checkOutDate;
        const bool blocksToday = getBookingState(*booking) == BookingState::ACTIVE
            && requestedCheckIn <= today && today < requestedCheckOut;
        if (overlaps || blocksToday) {
            unavailableRoomNumbers.insert(bookedRoom->getRoomNumber());
        }
    }

    std::vector<std::shared_ptr<Room>> availableRooms;
    availableRooms.reserve(rooms.size());
    for (const auto& room : rooms) {
        if (room && room->getIsAvailable() && !room->isArchived()
            && unavailableRoomNumbers.find(room->getRoomNumber()) == unavailableRoomNumbers.end()) {
            availableRooms.push_back(room);
        }
    }
    return availableRooms;
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
    booking->setCancelled(false);
    booking->setDeleted(false);
    booking->setCheckedIn(false);
    booking->setCheckedOut(false);
    booking->setAdultCount(adultCount);
    booking->setChildCount(childCount);
    const std::string createdTimestamp = QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toStdString();
    booking->setCreatedAt(createdTimestamp);
    booking->setUpdatedAt(createdTimestamp);
    booking->setQuotedUnitPrice(room->calculateTargetPrice());
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
    if (currentState == BookingState::COMPLETED) {
        errorMessage = "Cannot edit a completed booking.";
        return false;
    }
    if (!validateBookingDates(checkInDate, checkOutDate, errorMessage)) {
        return false;
    }

    const QDate proposedCheckIn = QDate::fromString(QString::fromStdString(checkInDate), Qt::ISODate);
    const QDate proposedCheckOut = QDate::fromString(QString::fromStdString(checkOutDate), Qt::ISODate);
    if (currentState == BookingState::UPCOMING && proposedCheckIn < QDate::currentDate()) {
        errorMessage = "Upcoming bookings cannot be moved to a past check-in date.";
        return false;
    }
    if (currentState == BookingState::ACTIVE) {
        if (checkInDate != booking->getCheckInDate() || roomNumber != booking->getRoom()->getRoomNumber()
            || customerId != booking->getCustomer()->getCustomerId()) {
            errorMessage = "Cannot change the guest, room, or check-in date after check-in.";
            return false;
        }
        if (proposedCheckOut < QDate::currentDate()) {
            errorMessage = "Cannot set an active booking's check-out date in the past.";
            return false;
        }
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
    booking->setCancelled(false);
    booking->setAdultCount(adultCount);
    booking->setChildCount(childCount);
    if (currentState == BookingState::UPCOMING) {
        booking->setQuotedUnitPrice(room->calculateTargetPrice());
    }
    booking->setUpdatedAt(QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toStdString());
    return true;
}

bool BookingManager::checkInBooking(const std::string& bookingId, const std::string& checkInDate,
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
    const QDate actualCheckIn = QDate::fromString(QString::fromStdString(checkInDate), Qt::ISODate);
    const QDate plannedCheckIn = QDate::fromString(QString::fromStdString(booking->getCheckInDate()), Qt::ISODate);
    const QDate plannedCheckOut = QDate::fromString(QString::fromStdString(booking->getCheckOutDate()), Qt::ISODate);
    if (!actualCheckIn.isValid() || actualCheckIn != QDate::currentDate()) {
        errorMessage = "Check-in must be recorded for today.";
        return false;
    }
    if (actualCheckIn < plannedCheckIn || actualCheckIn >= plannedCheckOut) {
        errorMessage = "Check-in must be on or after the planned arrival and before the planned departure.";
        return false;
    }
    booking->setCheckedIn(true);
    booking->setActualCheckInDate(checkInDate);
    booking->setUpdatedAt(QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toStdString());
    return true;
}

bool BookingManager::completeBooking(const std::string& bookingId,
                                     const std::string& checkoutDate,
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

    const QDate actualCheckout = QDate::fromString(QString::fromStdString(checkoutDate), Qt::ISODate);
    const QDate checkIn = QDate::fromString(QString::fromStdString(booking->getActualCheckInDate()), Qt::ISODate);
    if (!actualCheckout.isValid() || !checkIn.isValid()) {
        errorMessage = "The actual check-in and check-out dates must use ISO format (YYYY-MM-DD).";
        return false;
    }
    if (actualCheckout < checkIn) {
        errorMessage = "Checkout date cannot be before the actual check-in date.";
        return false;
    }
    if (actualCheckout > QDate::currentDate()) {
        errorMessage = "Checkout date cannot be in the future.";
        return false;
    }

    booking->setActualCheckOutDate(checkoutDate);
    booking->setCancelled(false);
    booking->setCheckedOut(true);
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

    const QDate plannedCheckIn = QDate::fromString(QString::fromStdString(booking->getCheckInDate()), Qt::ISODate);
    if (plannedCheckIn.isValid() && QDate::currentDate() >= plannedCheckIn) {
        errorMessage = "Reservations can only be cancelled before the planned check-in date. Mark the reservation as no-show after arrival day.";
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
    const auto booking = findBookingById(bookingId);
    if (!booking || booking->isDeleted() || booking->isCancelled() || booking->isCheckedIn()) {
        errorMessage = "Only an unarrived reservation can be marked as no-show.";
        return false;
    }
    const QDate plannedCheckIn = QDate::fromString(QString::fromStdString(booking->getCheckInDate()), Qt::ISODate);
    if (!plannedCheckIn.isValid() || QDate::currentDate() < plannedCheckIn) {
        errorMessage = "A reservation can only be marked as no-show on or after its planned check-in date.";
        return false;
    }

    booking->setCancelled(true);
    booking->setCancellationReason("No-show: " + (reason.empty() ? std::string("Guest did not arrive") : reason));
    booking->setCancelledAt(QDate::currentDate().toString(Qt::ISODate).toStdString());
    booking->setUpdatedAt(QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toStdString());
    return true;
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

    const QDate actualCheckIn = QDate::fromString(QString::fromStdString(booking->getActualCheckInDate()), Qt::ISODate);
    const QDate actualCheckOut = QDate::fromString(QString::fromStdString(booking->getActualCheckOutDate()), Qt::ISODate);
    if (!actualCheckIn.isValid() || !actualCheckOut.isValid() || actualCheckOut < actualCheckIn) {
        errorMessage = "The completed booking has an invalid actual stay duration.";
        return false;
    }
    const int derivedNights = actualCheckIn == actualCheckOut
        ? 1
        : static_cast<int>(actualCheckIn.daysTo(actualCheckOut));

    auto invoice = std::make_shared<Invoice>();
    invoice->setInvoiceId(invoiceId);
    invoice->setBookingId(bookingId);
    invoice->captureBookingSnapshot(booking);
    invoice->setTaxRate(booking->getQuotedTaxRate());
    invoice->setNights(derivedNights);
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
    if (!validateBookingDates(checkInDate, checkOutDate, errorMessage)) {
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
    if (nights <= 0) {
        errorMessage = "Stay duration in nights must be greater than zero.";
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
    if (!invoice->isValid()) {
        errorMessage = "Persisted invoice is invalid.";
        return false;
    }

    m_invoices.push_back(invoice);
    return true;
}

void BookingManager::clearAll()
{
    m_bookings.clear();
    m_invoices.clear();
}
