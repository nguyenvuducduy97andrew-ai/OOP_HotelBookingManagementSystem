#include "Invoice.h"
#include "Booking.h"
#include "Room.h"
#include "Customer.h"

#include <string>

Invoice::Invoice() {
    this->invoiceId = "INV_UNKNOWN";
    this->totalAmount = 0.0;
    this->taxRate = 0.0;
    this->paymentDate = QDate::currentDate();
}

std::string Invoice::getInvoiceId() const {
    return invoiceId;
}
void Invoice::setInvoiceId(const std::string& invoiceId) {
    this->invoiceId = invoiceId;
}

std::shared_ptr<Booking> Invoice::getBooking() const {
    // Lock the weak_ptr to get a shared_ptr; returns nullptr if expired
    return booking.lock();
}

void Invoice::setBooking(const std::shared_ptr<Booking>& booking) {
    // Store weak reference (does not increase refcount)
    this->booking = booking;
    // Recalculate total when booking is set
    if (booking != nullptr) {
        double subtotal = calculateSubtotal();
        totalAmount = subtotal * (1.0 + taxRate);
    }
    else {
        totalAmount = 0.0;
    }
}

void Invoice::setBooking(Booking* booking) {
    // Legacy raw pointer overload - cannot create weak_ptr from raw pointer safely
    this->booking = std::weak_ptr<Booking>();
    totalAmount = 0.0;
}

double Invoice::getTotalAmount() const {
    return totalAmount;
}
void Invoice::setTotalAmount(double totalAmount) {
    this->totalAmount = totalAmount;
}

double Invoice::getTaxRate() const {
    return taxRate;
}

void Invoice::setTaxRate(double taxRate) {
    this->taxRate = taxRate;
    // Recalculate total when tax rate is changed
    auto lockedBooking = booking.lock();
    if (lockedBooking != nullptr) {
        double subtotal = calculateSubtotal();
        totalAmount = subtotal * (1.0 + taxRate);
    }
}

QDate Invoice::getPaymentDate() const {
    return paymentDate;
}
void Invoice::setPaymentDate(const QDate& paymentDate) {
    this->paymentDate = paymentDate;
}

bool Invoice::isValid() const {
    // Lock the weak_ptr and check if booking is valid
    auto lockedBooking = booking.lock();
    return lockedBooking != nullptr && lockedBooking->isValid();
}

double Invoice::calculateSubtotal() const {
    auto lockedBooking = booking.lock();
    if (lockedBooking == nullptr)
        return 0.0;

    auto lockedRoom = lockedBooking->getRoom();
    if (lockedRoom == nullptr)
        return 0.0;

    int nights = lockedBooking->getDurationInNights();
    double roomPrice = lockedRoom->getBasePrice();
    return nights * roomPrice;
}

QString Invoice::generateInvoiceDetails() {
    QString details;

    details += "<h3>====== HOTEL BOOKING INVOICE ======</h3>";
    details += "<b>Invoice ID:</b> " + QString::fromStdString(invoiceId) + "<br>";
    details += "<b>Payment Date:</b> " + paymentDate.toString("dd/MM/yyyy") + "<br>";
    details += "--------------------------------------------------<br>";

    // Lock the weak_ptr to get a shared_ptr
    auto lockedBooking = booking.lock();

    // Error handling for missing or expired booking
    if (lockedBooking == nullptr) {
        details += "<font color='red'><b>Error: No booking data associated or booking has expired!</b></font><br>";
        details += "=========================================<br>";
        return details;
    }

    // Customer information
    auto lockedCustomer = lockedBooking->getCustomer();
    if (lockedCustomer != nullptr) {
        details += "<b>Customer Name:</b> " + QString::fromStdString(lockedCustomer->getName()) + "<br>";
    }
    else {
        details += "<b>Customer Name:</b> Unknown (or expired)<br>";
    }

    // Room information
    auto lockedRoom = lockedBooking->getRoom();
    if (lockedRoom != nullptr) {
        details += "<b>Room Number:</b> " + QString::fromStdString(lockedRoom->getRoomNumber()) + "<br>";
        details += "<b>Base Price per Night:</b> $" + QString::number(lockedRoom->getBasePrice(), 'f', 2) + "<br>";
    } else {
        details += "<b>Room Info:</b> Not Assigned (or expired)<br>";
    }

    details += "<b>Check-in Date:</b> " + lockedBooking->getCheckInDate().toString("dd/MM/yyyy") + "<br>";
    details += "<b>Check-out Date:</b> " + lockedBooking->getCheckOutDate().toString("dd/MM/yyyy") + "<br>";
    details += "<b>Duration:</b> " + QString::number(lockedBooking->getDurationInNights()) + " night(s)<br>";

    // Billing breakdown
    double subtotal = calculateSubtotal();
    details += "--------------------------------------------------<br>";
    details += "<b>Subtotal:</b> $" + QString::number(subtotal, 'f', 2) + "<br>";
    details += "<b>Tax Rate:</b> " + QString::number(taxRate * 100, 'f', 1) + "%<br>";
    details += "<b>Tax Amount:</b> $" + QString::number(subtotal * taxRate, 'f', 2) + "<br>";
    details += "--------------------------------------------------<br>";
    details += "<h2>TOTAL AMOUNT: $" + QString::number(totalAmount, 'f', 2) + "</h2>";
    details += "=========================================<br>";

    return details;
}