#include "InvoiceService.h"

#include "Booking.h"
#include "../../hotel/HotelManager.h"
#include "Invoice.h"

#include <QDate>
#include <QString>

InvoiceService::InvoiceService(HotelManager& hotelManager)
    : m_hotelManager(hotelManager)
{
}

bool InvoiceService::createInvoice(
    const std::string& invoiceId,
    const std::string& bookingId,
    double taxRate,
    int nights,
    const std::string& paymentDate,
    std::string& errorMessage)
{
    if (invoiceId.empty()) {
        errorMessage = "Invoice ID is required.";
        return false;
    }
    if (m_hotelManager.invoiceIdExists(invoiceId)) {
        errorMessage = "Invoice ID already exists.";
        return false;
    }
    if (taxRate < 0 || taxRate > 1) {
        errorMessage = "Tax rate must be between 0% and 100%.";
        return false;
    }
    if (nights <= 0) {
        errorMessage = "Stay duration in nights must be greater than zero.";
        return false;
    }
    const QDate payment = QDate::fromString(QString::fromStdString(paymentDate), Qt::ISODate);
    if (!payment.isValid()) {
        errorMessage = "Date must use ISO format (YYYY-MM-DD).";
        return false;
    }

    const auto booking = m_hotelManager.findBookingById(bookingId);
    if (!booking) {
        errorMessage = "Booking not found.";
        return false;
    }
    if (booking->isDeleted()) {
        errorMessage = "Cannot create invoice for a deleted booking.";
        return false;
    }
    if (m_hotelManager.getBookingState(*booking) != BookingState::COMPLETED) {
        errorMessage = "Invoice can only be created after checkout.";
        return false;
    }
    if (m_hotelManager.findInvoiceForBooking(bookingId)) {
        errorMessage = "An invoice already exists for this booking.";
        return false;
    }
    const QDate checkout = QDate::fromString(QString::fromStdString(booking->getCheckOutDate()), Qt::ISODate);
    // Modified: Reject impossible invoice dates before the invoice becomes part of the persisted financial history.
    if (!checkout.isValid() || payment < checkout || payment > QDate::currentDate()) {
        errorMessage = "Payment date must be between the actual checkout date and today.";
        return false;
    }

    auto invoice = std::make_shared<Invoice>();
    invoice->setInvoiceId(invoiceId);
    invoice->setBookingId(bookingId);
    invoice->captureBookingSnapshot(booking);
    invoice->setTaxRate(taxRate);
    invoice->setNights(nights);
    invoice->setPaymentDate(paymentDate);
    if (!invoice->isValid()) {
        errorMessage = "Failed to validate invoice details.";
        return false;
    }

    // Modified: Isolate invoice creation from booking storage while preserving one-invoice-per-booking validation.
    m_hotelManager.addInvoice(invoice);
    return true;
}
