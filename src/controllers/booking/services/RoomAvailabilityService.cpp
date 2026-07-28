#include "RoomAvailabilityService.h"

#include "Booking.h"
#include "../../hotel/HotelManager.h"
#include "Room.h"

#include <QDate>
#include <QString>
#include <unordered_set>

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

        if (m_hotelManager.getBookingState(*booking) == BookingState::COMPLETED) {
            continue;
        }

        if (!excludedBookingId.empty() && booking->getBookingId() == excludedBookingId) {
            continue;
        }

        const auto bookedRoom = booking->getRoom();
        if (!bookedRoom || bookedRoom->getRoomNumber() != roomNumber) {
            continue;
        }

        // Modified: Centralize active-stay conflict detection so every booking flow applies the same availability rule.
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

std::vector<std::shared_ptr<Room>> RoomAvailabilityService::getAvailableRoomsForDates(
    const std::string& checkInDate,
    const std::string& checkOutDate,
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
    for (const RoomMaintenance& maintenance : m_hotelManager.getRoomMaintenances()) {
        if (checkInDate < maintenance.getEndDate() && maintenance.getStartDate() < checkOutDate) {
            unavailableRoomNumbers.insert(maintenance.getRoomNumber());
        }
    }

    const QDate today = QDate::currentDate();
    for (const auto& booking : m_hotelManager.getBookings()) {
        if (!booking || booking->isCancelled() || booking->isDeleted() ||
            (!excludedBookingId.empty() && booking->getBookingId() == excludedBookingId)) {
            continue;
        }

        if (m_hotelManager.getBookingState(*booking) == BookingState::COMPLETED) {
            continue;
        }

        const auto bookedRoom = booking->getRoom();
        if (!bookedRoom) {
            continue;
        }

        const bool overlaps = checkInDate < booking->getCheckOutDate()
            && booking->getCheckInDate() < checkOutDate;
        const bool blocksToday = m_hotelManager.getBookingState(*booking) == BookingState::ACTIVE
            && requestedCheckIn <= today && today < requestedCheckOut;
        if (overlaps || blocksToday) {
            unavailableRoomNumbers.insert(bookedRoom->getRoomNumber());
        }
    }

    std::vector<std::shared_ptr<Room>> availableRooms;
    availableRooms.reserve(m_hotelManager.getRooms().size());
    for (const auto& room : m_hotelManager.getRooms()) {
        if (room && room->getIsAvailable() && !room->isArchived()
            && unavailableRoomNumbers.find(room->getRoomNumber()) == unavailableRoomNumbers.end()) {
            availableRooms.push_back(room);
        }
    }

    // Modified: Build booking and maintenance exclusions once, replacing the previous room-by-booking nested scan.
    return availableRooms;
}
