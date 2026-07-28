#pragma once

#include <string>

#include "services/BookingService.h"
#include "services/InvoiceService.h"

class HotelManager;

// Modified and optimized performance: own only the immediate booking and invoice services for the reservation domain.
class BookingManager
{
public:
    // Modified and optimized performance: keep booking and invoice services behind one direct reservation manager boundary.
    explicit BookingManager(HotelManager& hotelManager);

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
