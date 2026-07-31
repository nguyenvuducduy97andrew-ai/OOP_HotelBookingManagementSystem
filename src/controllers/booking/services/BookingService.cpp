#include "BookingService.h"

#include "Booking.h"
#include "Customer.h"
#include "../../hotel/HotelManager.h"
#include "Room.h"
#include "RoomAvailabilityService.h"

#include <QDate>
#include <QDateTime>
#include <QString>

BookingService::BookingService(HotelManager& hotelManager)
    : m_hotelManager(hotelManager)
{
}

bool BookingService::validateBookingDates(
    const std::string& checkInDate,
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

bool BookingService::validateBookingInput(
    const std::string& customerId,
    const std::string& roomNumber,
    const std::string& checkInDate,
    const std::string& checkOutDate,
    int adultCount,
    int childCount,
    std::string& errorMessage) const
{
    if (!validateBookingDates(checkInDate, checkOutDate, errorMessage)) {
        return false;
    }

    const auto customer = m_hotelManager.findCustomerById(customerId);
    if (!customer) {
        errorMessage = "Customer not found.";
        return false;
    }
    if (customer->isArchived()) {
        errorMessage = "Cannot create booking for an archived customer.";
        return false;
    }

    const auto room = m_hotelManager.findRoomByNumber(roomNumber);
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
    // Modified: Enforce room capacity in the service layer so every booking entry point follows the same safety rule.
    if (adultCount <= 0 || childCount < 0 || adultCount + childCount > room->getMaximumGuests()) {
        errorMessage = "Guest count must include at least one adult and cannot exceed this room's capacity of "
            + std::to_string(room->getMaximumGuests()) + ".";
        return false;
    }

    RoomAvailabilityService availability(m_hotelManager);
    return availability.isRoomFreeForDates(roomNumber, checkInDate, checkOutDate, errorMessage);
}

bool BookingService::createBooking(
    const std::string& customerId,
    const std::string& roomNumber,
    const std::string& checkInDate,
    const std::string& checkOutDate,
    int adultCount,
    int childCount,
    std::string& errorMessage)
{
    if (!validateBookingInput(customerId, roomNumber, checkInDate, checkOutDate, adultCount, childCount, errorMessage)) {
        return false;
    }

    const QDate checkIn = QDate::fromString(QString::fromStdString(checkInDate), Qt::ISODate);
    // Modified: New reservations may only start today or later; historical stays must be restored through persisted data.
    if (checkIn < QDate::currentDate()) {
        errorMessage = "New bookings cannot use a past check-in date.";
        return false;
    }

    auto booking = std::make_shared<Booking>();
    booking->setBookingId(Booking::nextBookingId());
    booking->setCustomer(m_hotelManager.findCustomerById(customerId));
    booking->setRoom(m_hotelManager.findRoomByNumber(roomNumber));
    booking->setCheckInDate(checkInDate);
    booking->setCheckOutDate(checkOutDate);
    booking->setCancelled(false);
    booking->setDeleted(false);
    booking->setCheckedIn(false);
    booking->setCheckedOut(false);
    booking->setAdultCount(adultCount);
    booking->setChildCount(childCount);
    // Modified: Stamp new reservations at the workflow boundary so audit timestamps are independent from UI screens.
    const std::string createdTimestamp = QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toStdString();
    booking->setCreatedAt(createdTimestamp);
    booking->setUpdatedAt(createdTimestamp);
    const auto room = booking->getRoom();
    // Modified: Capture the confirmed room rate and tax when a reservation is made, not at checkout.
    booking->setQuotedUnitPrice(room->calculateTargetPrice());
    booking->setQuotedTaxRate(0.10);
    m_hotelManager.addBooking(booking);
    return true;
}

bool BookingService::updateBooking(
    const std::string& bookingId,
    const std::string& customerId,
    const std::string& roomNumber,
    const std::string& checkInDate,
    const std::string& checkOutDate,
    int adultCount,
    int childCount,
    std::string& errorMessage)
{
    const auto booking = m_hotelManager.findBookingById(bookingId);
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
    const BookingState currentState = m_hotelManager.getBookingState(*booking);
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
    // Modified: Keep an active stay's arrival date immutable and prevent backdated departure edits.
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

    const auto customer = m_hotelManager.findCustomerById(customerId);
    if (!customer) {
        errorMessage = "Customer not found.";
        return false;
    }
    if (customer->isArchived()) {
        errorMessage = "Cannot update booking for an archived customer.";
        return false;
    }

    const auto room = m_hotelManager.findRoomByNumber(roomNumber);
    if (!room) {
        errorMessage = "Room not found.";
        return false;
    }
    if (!room->getIsAvailable()) {
        errorMessage = "Room is permanently unavailable for booking.";
        return false;
    }
    if (room->isArchived()) {
        errorMessage = "Cannot update booking for an archived room.";
        return false;
    }
    if (adultCount <= 0 || childCount < 0 || adultCount + childCount > room->getMaximumGuests()) {
        errorMessage = "Guest count must include at least one adult and cannot exceed this room's capacity of "
            + std::to_string(room->getMaximumGuests()) + ".";
        return false;
    }

    RoomAvailabilityService availability(m_hotelManager);
    if (!availability.isRoomFreeForDates(roomNumber, checkInDate, checkOutDate, errorMessage, bookingId)) {
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
        // Modified: Reconfirm the rate only when staff amend an upcoming reservation before check-in.
        booking->setQuotedUnitPrice(room->calculateTargetPrice());
    }
    // Modified: Record successful operational edits without changing the immutable creation timestamp.
    booking->setUpdatedAt(QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toStdString());
    return true;
}

bool BookingService::checkInBooking(const std::string& bookingId,
                                    const std::string& checkInDate,
                                    std::string& errorMessage)
{
    const auto booking = m_hotelManager.findBookingById(bookingId);
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
    // Modified: An operational check-in event, rather than the calendar date alone, activates room occupancy.
    booking->setCheckedIn(true);
    booking->setActualCheckInDate(checkInDate);
    booking->setUpdatedAt(QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toStdString());
    return true;
}

bool BookingService::completeBooking(
    const std::string& bookingId,
    const std::string& checkoutDate,
    std::string& errorMessage)
{
    const auto booking = m_hotelManager.findBookingById(bookingId);
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
    if (m_hotelManager.getBookingState(*booking) != BookingState::ACTIVE) {
        errorMessage = "Only active bookings can be checked out.";
        return false;
    }

    const QDate actualCheckout = QDate::fromString(QString::fromStdString(checkoutDate), Qt::ISODate);
    const QDate checkIn = QDate::fromString(QString::fromStdString(booking->getActualCheckInDate()), Qt::ISODate);
    if (!actualCheckout.isValid() || !checkIn.isValid()) {
        errorMessage = "The actual check-in and check-out dates must use ISO format (YYYY-MM-DD).";
        return false;
    }

    // Modified: Allow a same-day departure as a one-night stay while rejecting a checkout before the factual arrival.
    if (actualCheckout < checkIn) {
        errorMessage = "Checkout date cannot be before the actual check-in date.";
        return false;
    }
    if (actualCheckout > QDate::currentDate()) {
        errorMessage = "Checkout date cannot be in the future.";
        return false;
    }

    // Modified: Keep the planned departure immutable and record the factual departure independently for billing and audit.
    booking->setActualCheckOutDate(checkoutDate);
    booking->setCancelled(false);
    booking->setCheckedOut(true);
    booking->setUpdatedAt(QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toStdString());
    return true;
}

bool BookingService::cancelBooking(const std::string& bookingId, const std::string& reason, std::string& errorMessage)
{
    const auto booking = m_hotelManager.findBookingById(bookingId);
    if (!booking) {
        errorMessage = "Booking record not found.";
        return false;
    }
    if (booking->isDeleted()) {
        errorMessage = "This booking has already been deleted.";
        return false;
    }

    const BookingState state = m_hotelManager.getBookingState(*booking);
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

    // Modified: Preserve why and when a reservation was cancelled rather than deleting business history.
    booking->setCancelled(true);
    booking->setCancellationReason(reason.empty() ? "Cancelled by staff" : reason);
    booking->setCancelledAt(QDate::currentDate().toString(Qt::ISODate).toStdString());
    booking->setUpdatedAt(QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toStdString());
    return true;
}

bool BookingService::markNoShow(const std::string& bookingId, const std::string& reason, std::string& errorMessage)
{
    const auto booking = m_hotelManager.findBookingById(bookingId);
    if (!booking || booking->isDeleted() || booking->isCancelled() || booking->isCheckedIn()) {
        errorMessage = "Only an unarrived reservation can be marked as no-show.";
        return false;
    }
    const QDate plannedCheckIn = QDate::fromString(QString::fromStdString(booking->getCheckInDate()), Qt::ISODate);
    if (!plannedCheckIn.isValid() || QDate::currentDate() < plannedCheckIn) {
        errorMessage = "A reservation can only be marked as no-show on or after its planned check-in date.";
        return false;
    }
    // Modified: Record no-show separately from guest-initiated cancellation while releasing future inventory.
    booking->setCancelled(true);
    booking->setCancellationReason("No-show: " + (reason.empty() ? std::string("Guest did not arrive") : reason));
    booking->setCancelledAt(QDate::currentDate().toString(Qt::ISODate).toStdString());
    booking->setUpdatedAt(QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toStdString());
    return true;
}
