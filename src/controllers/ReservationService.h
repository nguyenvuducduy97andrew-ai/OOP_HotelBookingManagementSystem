#pragma once

#include <string>

#include "BookingService.h"
#include "InvoiceService.h"

class HotelManager;

// Reservation is the public operational boundary for stays: booking lifecycle, availability, checkout, and its invoice.
class ReservationService
{
public:
    explicit ReservationService(HotelManager& hotelManager);

    bool createBooking(const std::string& customerId, const std::string& roomNumber,
                       const std::string& checkInDate, const std::string& checkOutDate,
                       std::string& errorMessage);
    bool updateBooking(const std::string& bookingId, const std::string& customerId,
                       const std::string& roomNumber, const std::string& checkInDate,
                       const std::string& checkOutDate, std::string& errorMessage);
    bool cancelBooking(const std::string& bookingId, std::string& errorMessage);
    bool completeBooking(const std::string& bookingId, const std::string& checkoutDate,
                         std::string& errorMessage);
    bool createInvoice(const std::string& invoiceId, const std::string& bookingId,
                       double taxRate, int nights, const std::string& paymentDate,
                       std::string& errorMessage);

private:
    BookingService m_bookingWorkflow;
    InvoiceService m_invoiceWorkflow;
};
