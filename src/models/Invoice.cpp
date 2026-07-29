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
// Modified: invoice issue date defaults to an empty ISO value; payment settlement is not modeled here.
Invoice::Invoice() {
    this->invoiceId = "INV_UNKNOWN";
    this->bookingId = "";
    this->taxRate = 0.0;
    this->nights = 0; // Added: Initializing the new nights member variable
    this->invoiceIssuedDate = "";
    this->unitPrice = 0.0;
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
std::string Invoice::getInvoiceIssuedDate() const {
    return invoiceIssuedDate;
}

// Modified: Parameter type updated to const std::string&
void Invoice::setInvoiceIssuedDate(const std::string& value) {
    invoiceIssuedDate = value;
}

double Invoice::getUnitPrice() const { return unitPrice; }
void Invoice::setUnitPrice(double value) { unitPrice = value; }
std::string Invoice::getCustomerNameSnapshot() const { return customerNameSnapshot; }
void Invoice::setCustomerNameSnapshot(const std::string& value) { customerNameSnapshot = value; }
std::string Invoice::getCustomerIdSnapshot() const { return customerIdSnapshot; }
void Invoice::setCustomerIdSnapshot(const std::string& value) { customerIdSnapshot = value; }
std::string Invoice::getCustomerPhoneSnapshot() const { return customerPhoneSnapshot; }
void Invoice::setCustomerPhoneSnapshot(const std::string& value) { customerPhoneSnapshot = value; }
std::string Invoice::getRoomNumberSnapshot() const { return roomNumberSnapshot; }
void Invoice::setRoomNumberSnapshot(const std::string& value) { roomNumberSnapshot = value; }
std::string Invoice::getRoomTypeSnapshot() const { return roomTypeSnapshot; }
void Invoice::setRoomTypeSnapshot(const std::string& value) { roomTypeSnapshot = value; }
std::string Invoice::getCheckInDateSnapshot() const { return checkInDateSnapshot; }
void Invoice::setCheckInDateSnapshot(const std::string& value) { checkInDateSnapshot = value; }
std::string Invoice::getCheckOutDateSnapshot() const { return checkOutDateSnapshot; }
void Invoice::setCheckOutDateSnapshot(const std::string& value) { checkOutDateSnapshot = value; }

// Modified: Validates booking existence, invoice data integrity, and duration validity
bool Invoice::isValid() const {
    // Modified: Require a linked booking, complete immutable snapshots, and valid billing values.
    const auto lockedBooking = booking.lock();

    return !invoiceId.empty() &&
           lockedBooking != nullptr &&
           nights > 0 &&
           taxRate >= 0.0 && taxRate <= 1.0 &&
           unitPrice > 0.0 &&
           !customerNameSnapshot.empty() && !customerIdSnapshot.empty() && !customerPhoneSnapshot.empty() &&
           !roomNumberSnapshot.empty() && !roomTypeSnapshot.empty() &&
           isIsoDateString(invoiceIssuedDate) && isIsoDateString(checkInDateSnapshot) &&
           isIsoDateString(checkOutDateSnapshot);
}

// Modified: Calculate subtotal from the stored stay length and immutable unit price.
double Invoice::calculateSubtotal() const {
    return nights * unitPrice;
}

// Modified: Calculate the final total from the immutable subtotal and stored tax rate.
double Invoice::calculateTotal() const {
    return calculateSubtotal() * (1.0 + taxRate);
}

// Modified: Refactored entirely to return std::string instead of QString using std::stringstream
std::string Invoice::generateInvoiceDetails() const {
    // Fixed-modified: Keep invoice rendering self-contained while formatting money without Qt locale APIs.
    std::stringstream details;

    details << "<h3>====== HOTEL BOOKING INVOICE ======</h3>";
    details << "<b>Invoice ID:</b> " << invoiceId << "<br>";
    details << "<b>Invoice Issued Date:</b> " << invoiceIssuedDate << "<br>";
    details << "--------------------------------------------------<br>";

    // Modified: Render the persisted snapshot so later customer or room edits cannot change a historical invoice.
    details << "<b>Booking ID:</b> " << bookingId << "<br>";
    details << "<b>Customer Name:</b> " << customerNameSnapshot << "<br>";
    details << "<b>Customer ID:</b> " << customerIdSnapshot << "<br>";
    details << "<b>Phone:</b> " << customerPhoneSnapshot << "<br>";
    details << "<b>Room:</b> " << roomNumberSnapshot << " (" << roomTypeSnapshot << ")<br>";
    details << "<b>Price per Night:</b> " << formatMoney(unitPrice) << " VND<br>";
    details << "<b>Check-in Date:</b> " << checkInDateSnapshot << "<br>";
    details << "<b>Check-out Date:</b> " << checkOutDateSnapshot << "<br>";
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

void Invoice::captureBookingSnapshot(const std::shared_ptr<Booking>& sourceBooking) {
    setBooking(sourceBooking);
    if (!sourceBooking) {
        return;
    }

    // Modified: Capture immutable guest, room, date, and pricing data when an invoice is created.
    // Modified: Bill from confirmed reservation values and actual stay dates, never from mutable room pricing at checkout.
    checkInDateSnapshot = sourceBooking->getActualCheckInDate().empty()
        ? sourceBooking->getCheckInDate() : sourceBooking->getActualCheckInDate();
    checkOutDateSnapshot = sourceBooking->getEffectiveCheckOutDate();
    unitPrice = sourceBooking->getQuotedUnitPrice();
    const auto customer = sourceBooking->getCustomer();
    if (customer) {
        customerNameSnapshot = customer->getName();
        customerIdSnapshot = customer->getCustomerId();
        customerPhoneSnapshot = customer->getPhoneNumber();
    }
    const auto room = sourceBooking->getRoom();
    if (room) {
        roomNumberSnapshot = room->getRoomNumber();
        roomTypeSnapshot = room->getRoomTypeName();
    }
}
