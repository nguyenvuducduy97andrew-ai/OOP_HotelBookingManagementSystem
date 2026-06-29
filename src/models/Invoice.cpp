#include "Invoice.h"
#include "Booking.h"
#include "Room.h"
#include "Customer.h"
#include <iostream>
#include <iomanip>
#include <sstream> // Added for std::stringstream in generateInvoiceDetails

using namespace std;

// Modified: Updated default constructor to initialize 'nights' instead of 'totalAmount'
// Modified: paymentDate now defaults to an empty string or dummy ISO format
Invoice::Invoice() {
    this->invoiceId = "INV_UNKNOWN";
    this->taxRate = 0.0;
    this->nights = 0; // Added: Initializing the new nights member variable
    this->paymentDate = ""; // Modified: Using empty string instead of QDate::currentDate()
}

string Invoice::getInvoiceId() const {
    return invoiceId;
}
void Invoice::setInvoiceId(const string& invoiceId) {
    this->invoiceId = invoiceId;
}

std::shared_ptr<Booking> Invoice::getBooking() const {
    // Lock the weak_ptr to get a shared_ptr; returns nullptr if expired
    return booking.lock();
}

// Modified: Removed totalAmount recalculation logic since it's computed dynamically now
void Invoice::setBooking(const std::shared_ptr<Booking>& booking) {
    // Store weak reference (does not increase refcount)
    this->booking = booking;
}

// Modified: Cleaned up raw pointer overload to match the dynamic calculation architecture
void Invoice::setBooking(Booking* booking) {
    // Legacy raw pointer overload - cannot create weak_ptr from raw pointer safely
    this->booking = std::weak_ptr<Booking>();
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
string Invoice::getPaymentDate() const {
    return paymentDate;
}

// Modified: Parameter type updated to const std::string&
void Invoice::setPaymentDate(const string& paymentDate) {
    this->paymentDate = paymentDate;
}

// Modified: Validates booking existence, invoice data integrity, and duration validity
bool Invoice::isValid() const {
    // Lock the weak_ptr and check if booking is valid
    auto lockedBooking = booking.lock();
    return lockedBooking != nullptr && !invoiceId.empty() && nights > 0;
}

// Modified: Computes subtotal on-the-fly using the locally stored 'nights' variable
double Invoice::calculateSubtotal() const {
    auto lockedBooking = booking.lock();
    if (lockedBooking == nullptr)
        return 0.0;

    auto lockedRoom = lockedBooking->getRoom();
    if (lockedRoom == nullptr)
        return 0.0;

    double roomPrice = lockedRoom->getBasePrice();
    return nights * roomPrice; // Modified: Using the nights property passed from View
}

// Added: Computes final total including tax rate dynamically
double Invoice::calculateTotal() const {
    return calculateSubtotal() * (1.0 + taxRate);
}

// Modified: Refactored entirely to return std::string instead of QString using std::stringstream
std::string Invoice::generateInvoiceDetails() const {
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
        details << "<b>Base Price per Night:</b> $" << std::fixed << std::setprecision(2) << lockedRoom->getBasePrice() << "<br>";
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
    details << "<b>Subtotal:</b> $" << std::fixed << std::setprecision(2) << subtotal << "<br>";
    details << "<b>Tax Rate:</b> " << std::fixed << std::setprecision(1) << (taxRate * 100) << "%<br>";
    details << "<b>Tax Amount:</b> $" << std::fixed << std::setprecision(2) << taxAmount << "<br>";
    details << "--------------------------------------------------<br>";
    details << "<h2>TOTAL AMOUNT: $" << std::fixed << std::setprecision(2) << totalAmount << "</h2>";
    details << "=========================================<br>";

    return details.str();
}