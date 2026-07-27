#pragma once

#include <string>

class HotelManager;

class BookingService
{
public:
    explicit BookingService(HotelManager& hotelManager);

    bool createBooking(
        const std::string& customerId,
        const std::string& roomNumber,
        const std::string& checkInDate,
        const std::string& checkOutDate,
        std::string& errorMessage);

    bool updateBooking(
        const std::string& bookingId,
        const std::string& customerId,
        const std::string& roomNumber,
        const std::string& checkInDate,
        const std::string& checkOutDate,
        std::string& errorMessage);

    bool completeBooking(
        const std::string& bookingId,
        const std::string& checkoutDate,
        std::string& errorMessage);

    bool cancelBooking(const std::string& bookingId, std::string& errorMessage);

private:
    bool validateBookingDates(
        const std::string& checkInDate,
        const std::string& checkOutDate,
        std::string& errorMessage) const;
    bool validateBookingInput(
        const std::string& customerId,
        const std::string& roomNumber,
        const std::string& checkInDate,
        const std::string& checkOutDate,
        std::string& errorMessage) const;

    HotelManager& m_hotelManager;
};
