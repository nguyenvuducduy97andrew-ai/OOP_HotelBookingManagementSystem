#include "Invoice.h"
#include "Booking.h"
#include "Room.h"
#include "Customer.h"
#include <iostream>
#include <iomanip>
#include <sstream> // Added for std::stringstream in generateInvoiceDetails
#include <locale>

namespace {
// Fixed-modified: Replace Qt-only helpers with standard C++ parsing/formatting in the invoice model.
bool isIsoDateString(const std::string& value)
{
    std::tm parsed{};
    std::istringstream input(value);
    input >> std::get_time(&parsed, "%Y-%m-%d");
    return !input.fail() && input.eof();
}

std::string formatMoney(double value)
{
    std::ostringstream output;
    output.imbue(std::locale::classic());
    output << std::fixed << std::setprecision(0) << value;
    return output.str();
}
}


// Modified: Updated default constructor to initialize 'nights' instead of 'totalAmount'
// Modified: paymentDate now defaults to an empty string or dummy ISO format
Invoice::Invoice() {
    this->invoiceId = "INV_UNKNOWN";
    this->bookingId = "";
    this->taxRate = 0.0;
    this->nights = 0; // Added: Initializing the new nights member variable
    this->paymentDate = ""; // Modified: Using empty string instead of QDate::currentDate()
}

std::string Invoice::getInvoiceId() const {
    return invoiceId;
}
void Invoice::setInvoiceId(const std::string& invoiceId) {
    this->invoiceId = invoiceId;
}

std::string Invoice::getBookingId() const {
    return bookingId;
}

void Invoice::setBookingId(const std::string& bookingId) {
    this->bookingId = bookingId;
}

std::shared_ptr<Booking> Invoice::getBooking() const {
    // Lock the weak_ptr to get a shared_ptr; returns nullptr if expired
    return booking.lock();
}

// Modified: Removed totalAmount recalculation logic since it's computed dynamically now
void Invoice::setBooking(const std::shared_ptr<Booking>& booking) {
    // Store weak reference (does not increase refcount)
    this->booking = booking;
    if (booking) {
        this->bookingId = booking->getBookingId();
    }
}

double Invoice::getTaxRate() const {
    return taxRate;
}

// Modified: Removed totalAmount recalculation; setter simply updates the taxRate
void Invoice::setTaxRate(double taxRate) {
    this->taxRate = taxRate;
}

// Added: Getter implementation for nights
int Invoice::getNights() const {
    return nights;
}

// Added: Setter implementation for nights
void Invoice::setNights(int nights) {
    this->nights = nights;
}

// Modified: Returns std::string instead of QDate
std::string Invoice::getPaymentDate() const {
    return paymentDate;
}

// Modified: Parameter type updated to const std::string&
void Invoice::setPaymentDate(const std::string& paymentDate) {
    this->paymentDate = paymentDate;
}

// Modified: Validates booking existence, invoice data integrity, and duration validity
bool Invoice::isValid() const {
    // Fixed-modified: Require a linked booking, positive stay length, and a real ISO payment date.
    const auto lockedBooking = booking.lock();

    return !invoiceId.empty() &&
           lockedBooking != nullptr &&
           nights > 0 &&
           isIsoDateString(paymentDate);
}

// Modified: Computes subtotal on-the-fly using the locally stored 'nights' variable
double Invoice::calculateSubtotal() const {
    auto lockedBooking = booking.lock();
    if (lockedBooking == nullptr)
        return 0.0;

    auto lockedRoom = lockedBooking->getRoom();
    if (lockedRoom == nullptr)
        return 0.0;

    return nights * lockedRoom->calculateTargetPrice();
}

// Added: Computes final total including tax rate dynamically
double Invoice::calculateTotal() const {
    return calculateSubtotal() * (1.0 + taxRate);
}

// Modified: Refactored entirely to return std::string instead of QString using std::stringstream
std::string Invoice::generateInvoiceDetails() const {
    // Fixed-modified: Keep invoice rendering self-contained while formatting money without Qt locale APIs.
    std::stringstream details;

    details << "<h3>====== HOTEL BOOKING INVOICE ======</h3>";
    details << "<b>Invoice ID:</b> " << invoiceId << "<br>";
    details << "<b>Payment Date:</b> " << paymentDate << "<br>"; // Expected format: YYYY-MM-DD
    details << "--------------------------------------------------<br>";

    // Lock the weak_ptr to get a shared_ptr
    auto lockedBooking = booking.lock();

    // Error handling for missing or expired booking
    if (lockedBooking == nullptr) {
        details << "<font color='red'><b>Error: No booking data associated or booking has expired!</b></font><br>";
        if (!bookingId.empty()) {
            details << "<b>Booking ID:</b> " << bookingId << "<br>";
        }
        details << "=========================================<br>";
        return details.str();
    }

    // Customer information
    auto lockedCustomer = lockedBooking->getCustomer();
    if (lockedCustomer != nullptr) {
        details << "<b>Customer Name:</b> " << lockedCustomer->getName() << "<br>";
    }
    else {
        details << "<b>Customer Name:</b> Unknown (or expired)<br>";
    }

    // Room information
    auto lockedRoom = lockedBooking->getRoom();
    if (lockedRoom != nullptr) {
        details << "<b>Room Number:</b> " << lockedRoom->getRoomNumber() << "<br>";
        details << "<b>Base Price per Night:</b> " << formatMoney(lockedRoom->getBasePrice()) << " VND<br>";
    } else {
        details << "<b>Room Info:</b> Not Assigned (or expired)<br>";
    }

    // Modified: Booking schedule output formatted via direct string read
    details << "<b>Check-in Date:</b> " << lockedBooking->getCheckInDate() << "<br>";
    details << "<b>Check-out Date:</b> " << lockedBooking->getCheckOutDate() << "<br>";
    details << "<b>Duration:</b> " << nights << " night(s)<br>";

    // Billing breakdown
    double subtotal = calculateSubtotal();
    double taxAmount = subtotal * taxRate;
    double totalAmount = calculateTotal(); // Dynamic calculation

    details << "--------------------------------------------------<br>";
    details << "<b>Subtotal:</b> " << formatMoney(subtotal) << " VND<br>";
    details << "<b>Tax Rate:</b> " << std::fixed << std::setprecision(1) << (taxRate * 100) << "%<br>";
    details << "<b>Tax Amount:</b> " << formatMoney(taxAmount) << " VND<br>";
    details << "--------------------------------------------------<br>";
    details << "<h2>TOTAL AMOUNT: " << formatMoney(totalAmount) << " VND</h2>";
    details << "=========================================<br>";

    return details.str();
}
