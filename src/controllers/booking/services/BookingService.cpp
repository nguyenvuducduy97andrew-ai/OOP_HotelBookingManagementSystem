#include "BookingService.h"

#include "Booking.h"
#include "Customer.h"
#include "../../hotel/HotelManager.h"
#include "Room.h"
#include "RoomAvailabilityService.h"

#include <QDate>
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

    RoomAvailabilityService availability(m_hotelManager);
    return availability.isRoomFreeForDates(roomNumber, checkInDate, checkOutDate, errorMessage);
}

bool BookingService::createBooking(
    const std::string& customerId,
    const std::string& roomNumber,
    const std::string& checkInDate,
    const std::string& checkOutDate,
    std::string& errorMessage)
{
    if (!validateBookingInput(customerId, roomNumber, checkInDate, checkOutDate, errorMessage)) {
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
    booking->setCheckedOut(false);
    m_hotelManager.addBooking(booking);
    return true;
}

bool BookingService::updateBooking(
    const std::string& bookingId,
    const std::string& customerId,
    const std::string& roomNumber,
    const std::string& checkInDate,
    const std::string& checkOutDate,
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
        if (checkInDate != booking->getCheckInDate()) {
            errorMessage = "Cannot change the check-in date after the guest has checked in.";
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

    RoomAvailabilityService availability(m_hotelManager);
    if (!availability.isRoomFreeForDates(roomNumber, checkInDate, checkOutDate, errorMessage, bookingId)) {
        return false;
    }

    booking->setCustomer(customer);
    booking->setRoom(room);
    booking->setCheckInDate(checkInDate);
    booking->setCheckOutDate(checkOutDate);
    booking->setCancelled(false);
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
    const QDate checkIn = QDate::fromString(QString::fromStdString(booking->getCheckInDate()), Qt::ISODate);
    if (!actualCheckout.isValid()) {
        errorMessage = "Date must use ISO format (YYYY-MM-DD).";
        return false;
    }

    // Modified: Keep checkout validation and state mutation in the booking use case instead of the shared data manager.
    if (actualCheckout < checkIn) {
        errorMessage = "Checkout date cannot be before check-in date.";
        return false;
    }
    if (actualCheckout > QDate::currentDate()) {
        errorMessage = "Checkout date cannot be in the future.";
        return false;
    }

    booking->setCheckOutDate(checkoutDate);
    booking->setCancelled(false);
    booking->setCheckedOut(true);
    return true;
}

bool BookingService::cancelBooking(const std::string& bookingId, std::string& errorMessage)
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

    booking->setCancelled(true);
    return true;
}
