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
    const std::string& invoiceIssuedDate,
    const std::string& paymentMethod,
    double paymentAmount,
    const std::string& paymentReceivedDate,
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
    const QDate issuedDate = QDate::fromString(QString::fromStdString(invoiceIssuedDate), Qt::ISODate);
    if (!issuedDate.isValid()) {
        errorMessage = "Date must use ISO format (YYYY-MM-DD).";
        return false;
    }
    const QDate receivedDate = QDate::fromString(QString::fromStdString(paymentReceivedDate), Qt::ISODate);
    if (paymentMethod.empty() || paymentAmount <= 0.0 || !receivedDate.isValid() || receivedDate > QDate::currentDate()) {
        errorMessage = "A positive payment amount, method, and valid received date are required at checkout.";
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
    const QDate checkout = QDate::fromString(QString::fromStdString(booking->getActualCheckOutDate()), Qt::ISODate);
    // Modified: Reject impossible invoice dates before the invoice becomes part of the persisted financial history.
    if (!checkout.isValid() || issuedDate < checkout || issuedDate > QDate::currentDate()) {
        errorMessage = "Invoice issue date must be between the actual checkout date and today.";
        return false;
    }

    const QDate actualCheckIn = QDate::fromString(QString::fromStdString(booking->getActualCheckInDate()), Qt::ISODate);
    const QDate actualCheckOut = QDate::fromString(QString::fromStdString(booking->getActualCheckOutDate()), Qt::ISODate);
    const int derivedNights = actualCheckIn.daysTo(actualCheckOut);
    // Modified: Derive invoice nights and tax from the confirmed booking snapshot rather than accepting UI-supplied billing values.
    if (!actualCheckIn.isValid() || !actualCheckOut.isValid() || derivedNights <= 0) {
        errorMessage = "The completed booking has an invalid actual stay duration.";
        return false;
    }

    auto invoice = std::make_shared<Invoice>();
    invoice->setInvoiceId(invoiceId);
    invoice->setBookingId(bookingId);
    invoice->captureBookingSnapshot(booking);
    invoice->setTaxRate(booking->getQuotedTaxRate());
    invoice->setNights(derivedNights);
    invoice->setInvoiceIssuedDate(invoiceIssuedDate);
    // Modified: Persist the mandatory checkout payment separately from the invoice issue date.
    invoice->setPaymentMethod(paymentMethod);
    invoice->setPaymentAmount(paymentAmount);
    invoice->setPaymentReceivedDate(paymentReceivedDate);
    if (paymentAmount > invoice->calculateTotal()) {
        errorMessage = "Payment amount cannot exceed the invoice total in the current single-payment workflow.";
        return false;
    }
    if (!invoice->isValid()) {
        errorMessage = "Failed to validate invoice details.";
        return false;
    }

    // Modified: Isolate invoice creation from booking storage while preserving one-invoice-per-booking validation.
    m_hotelManager.addInvoice(invoice);
    return true;
}
