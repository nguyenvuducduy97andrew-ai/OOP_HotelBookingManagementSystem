#include "ReservationService.h"

#include "HotelManager.h"

ReservationService::ReservationService(HotelManager& hotelManager)
    : m_bookingWorkflow(hotelManager)
    , m_invoiceWorkflow(hotelManager)
{
}

bool ReservationService::createBooking(const std::string& customerId, const std::string& roomNumber,
                                       const std::string& checkInDate, const std::string& checkOutDate,
                                       std::string& errorMessage)
{
    return m_bookingWorkflow.createBooking(customerId, roomNumber, checkInDate, checkOutDate, errorMessage);
}

bool ReservationService::updateBooking(const std::string& bookingId, const std::string& customerId,
                                       const std::string& roomNumber, const std::string& checkInDate,
                                       const std::string& checkOutDate, std::string& errorMessage)
{
    return m_bookingWorkflow.updateBooking(bookingId, customerId, roomNumber, checkInDate, checkOutDate, errorMessage);
}

bool ReservationService::cancelBooking(const std::string& bookingId, std::string& errorMessage)
{
    return m_bookingWorkflow.cancelBooking(bookingId, errorMessage);
}

bool ReservationService::completeBooking(const std::string& bookingId, const std::string& checkoutDate,
                                         std::string& errorMessage)
{
    return m_bookingWorkflow.completeBooking(bookingId, checkoutDate, errorMessage);
}

bool ReservationService::createInvoice(const std::string& invoiceId, const std::string& bookingId,
                                       double taxRate, int nights, const std::string& paymentDate,
                                       std::string& errorMessage)
{
    // Modified and optimized performance: expose one reservation workflow for checkout and its invoice instead of separate UI-facing services.
    return m_invoiceWorkflow.createInvoice(invoiceId, bookingId, taxRate, nights, paymentDate, errorMessage);
}
