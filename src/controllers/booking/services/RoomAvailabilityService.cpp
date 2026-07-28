#include "RoomAvailabilityService.h"

#include "Booking.h"
#include "../../hotel/HotelManager.h"
#include "Room.h"

#include <QDate>
#include <QString>

RoomAvailabilityService::RoomAvailabilityService(const HotelManager& hotelManager)
    : m_hotelManager(hotelManager)
{
}

bool RoomAvailabilityService::isRoomFreeForDates(
    const std::string& roomNumber,
    const std::string& checkInDate,
    const std::string& checkOutDate,
    std::string& errorMessage,
    const std::string& excludedBookingId) const
{
    const QDate requestedCheckIn = QDate::fromString(QString::fromStdString(checkInDate), Qt::ISODate);
    const QDate requestedCheckOut = QDate::fromString(QString::fromStdString(checkOutDate), Qt::ISODate);
    const QDate today = QDate::currentDate();

    if (m_hotelManager.hasRoomMaintenanceConflict(roomNumber, checkInDate, checkOutDate, errorMessage)) {
        return false;
    }

    for (const auto& booking : m_hotelManager.getBookings()) {
        if (!booking || booking->isCancelled() || booking->isDeleted()) {
            continue;
        }

        if (!excludedBookingId.empty() && booking->getBookingId() == excludedBookingId) {
            continue;
        }

        const auto bookedRoom = booking->getRoom();
        if (!bookedRoom || bookedRoom->getRoomNumber() != roomNumber) {
            continue;
        }

        // Modified and optimized performance: centralize active-stay conflict detection so every booking flow applies the same availability rule.
        if (m_hotelManager.getBookingState(*booking) == BookingState::ACTIVE
            && requestedCheckIn <= today && today < requestedCheckOut) {
            errorMessage = "Room " + roomNumber + " still has an active stay that must be checked out first.";
            return false;
        }

        const bool overlaps = checkInDate < booking->getCheckOutDate()
            && booking->getCheckInDate() < checkOutDate;
        if (overlaps) {
            errorMessage = "Room " + roomNumber + " is already booked from "
                + booking->getCheckInDate() + " to " + booking->getCheckOutDate()
                + " (" + bookingStateToString(m_hotelManager.getBookingState(*booking)) + ").";
            return false;
        }
    }

    return true;
}
