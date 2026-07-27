#pragma once

#include <string>

class HotelManager;

class RoomAvailabilityService
{
public:
    explicit RoomAvailabilityService(const HotelManager& hotelManager);

    bool isRoomFreeForDates(
        const std::string& roomNumber,
        const std::string& checkInDate,
        const std::string& checkOutDate,
        std::string& errorMessage,
        const std::string& excludedBookingId = "") const;

private:
    const HotelManager& m_hotelManager;
};
