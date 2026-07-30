#include "BookingManager.h"

#include "../hotel/HotelManager.h"

BookingManager::BookingManager(HotelManager& hotelManager)
    : m_bookingWorkflow(hotelManager)
    , m_invoiceWorkflow(hotelManager)
{
}

bool BookingManager::createBooking(const std::string& customerId, const std::string& roomNumber,
                                       const std::string& checkInDate, const std::string& checkOutDate,
                                       int adultCount, int childCount,
                                       std::string& errorMessage)
{
    return m_bookingWorkflow.createBooking(customerId, roomNumber, checkInDate, checkOutDate,
                                           adultCount, childCount, errorMessage);
}

bool BookingManager::updateBooking(const std::string& bookingId, const std::string& customerId,
                                       const std::string& roomNumber, const std::string& checkInDate,
                                       const std::string& checkOutDate, int adultCount, int childCount,
                                       std::string& errorMessage)
{
    return m_bookingWorkflow.updateBooking(bookingId, customerId, roomNumber, checkInDate, checkOutDate,
                                           adultCount, childCount, errorMessage);
}

bool BookingManager::checkInBooking(const std::string& bookingId, const std::string& checkInDate,
                                    std::string& errorMessage)
{
    return m_bookingWorkflow.checkInBooking(bookingId, checkInDate, errorMessage);
}

bool BookingManager::cancelBooking(const std::string& bookingId, const std::string& reason, std::string& errorMessage)
{
    return m_bookingWorkflow.cancelBooking(bookingId, reason, errorMessage);
}

bool BookingManager::markNoShow(const std::string& bookingId, const std::string& reason, std::string& errorMessage)
{
    return m_bookingWorkflow.markNoShow(bookingId, reason, errorMessage);
}

bool BookingManager::completeBooking(const std::string& bookingId, const std::string& checkoutDate,
                                         std::string& errorMessage)
{
    return m_bookingWorkflow.completeBooking(bookingId, checkoutDate, errorMessage);
}

bool BookingManager::createInvoice(const std::string& invoiceId, const std::string& bookingId,
                                   const std::string& invoiceIssuedDate,
                                   const std::string& paymentMethod, double paymentAmount,
                                   const std::string& paymentReceivedDate,
                                   std::string& errorMessage)
{
    // Modified and optimized performance: expose one reservation workflow for checkout and its invoice instead of separate UI-facing services.
    return m_invoiceWorkflow.createInvoice(invoiceId, bookingId, invoiceIssuedDate,
                                           paymentMethod, paymentAmount, paymentReceivedDate, errorMessage);
}
