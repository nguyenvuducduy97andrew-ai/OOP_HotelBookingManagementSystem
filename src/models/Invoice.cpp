#include "Invoice.h"
#include "Booking.h"
#include "Room.h"
#include "Customer.h"
#include <cmath>
#include <iostream>
#include <iomanip>
#include <sstream> // Added for std::stringstream in generateInvoiceDetails

namespace {
// Fixed-modified: Replace Qt-only helpers with standard C++ parsing/formatting in the invoice model.
bool isIsoDateString(const std::string& value)
{
    std::tm parsed{};
    std::istringstream input(value);
    input >> std::get_time(&parsed, "%Y-%m-%d");
    return !input.fail() && input.eof();
}

// Modified: Format invoice amounts locally with comma thousands separators while keeping this model independent of Qt.
std::string formatMoney(double value)
{
    if (!std::isfinite(value)) {
        return "0";
    }

    const long long roundedValue = std::llround(value);
    const bool isNegative = roundedValue < 0;
    std::string digits = std::to_string(isNegative ? -roundedValue : roundedValue);
    for (std::size_t position = digits.length(); position > 3; position -= 3) {
        digits.insert(position - 3, 1, ',');
    }
    return isNegative ? "-" + digits : digits;
}

}


// Modified: Updated default constructor to initialize 'nights' instead of 'totalAmount'
// Modified: invoice issue date defaults to an empty ISO value; payment settlement is not modeled here.
Invoice::Invoice() {
    this->invoiceId = "INV_UNKNOWN";
    this->bookingId = "";
    this->taxRate = 0.0;
    this->nights = 0; // Added: Initializing the new nights member variable
    // Modified: Initialize time-based invoice facts as legacy-safe until the hourly checkout workflow is enabled.
    this->actualDurationSeconds = 0;
    this->billableHours = 0;
    this->legacyNightlyBilling = true;
    this->invoiceIssuedDate = "";
    this->paymentMethod = "";
    this->paymentAmount = 0.0;
    this->paymentReceivedDate = "";
    this->unitPrice = 0.0;
    this->hourlyRoomRateSnapshot = 0.0;
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

long long Invoice::getActualDurationSeconds() const { return actualDurationSeconds; }
void Invoice::setActualDurationSeconds(long long value) { actualDurationSeconds = value; }
int Invoice::getBillableHours() const { return billableHours; }
void Invoice::setBillableHours(int value) { billableHours = value; }
bool Invoice::usesLegacyNightlyBilling() const { return legacyNightlyBilling; }
void Invoice::setLegacyNightlyBilling(bool value) { legacyNightlyBilling = value; }

// Modified: Returns std::string instead of QDate
std::string Invoice::getInvoiceIssuedDate() const {
    return invoiceIssuedDate;
}

// Modified: Parameter type updated to const std::string&
void Invoice::setInvoiceIssuedDate(const std::string& value) {
    invoiceIssuedDate = value;
}

std::string Invoice::getPaymentMethod() const { return paymentMethod; }
void Invoice::setPaymentMethod(const std::string& value) { paymentMethod = value; }
double Invoice::getPaymentAmount() const { return paymentAmount; }
void Invoice::setPaymentAmount(double value) { paymentAmount = value; }
std::string Invoice::getPaymentReceivedDate() const { return paymentReceivedDate; }
void Invoice::setPaymentReceivedDate(const std::string& value) { paymentReceivedDate = value; }

double Invoice::getUnitPrice() const { return unitPrice; }
void Invoice::setUnitPrice(double value) { unitPrice = value; }
double Invoice::getHourlyRoomRateSnapshot() const { return hourlyRoomRateSnapshot; }
void Invoice::setHourlyRoomRateSnapshot(double value) { hourlyRoomRateSnapshot = value; }
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
std::string Invoice::getPlannedCheckInAtSnapshot() const { return plannedCheckInAtSnapshot; }
void Invoice::setPlannedCheckInAtSnapshot(const std::string& value) { plannedCheckInAtSnapshot = value; }
std::string Invoice::getPlannedCheckOutAtSnapshot() const { return plannedCheckOutAtSnapshot; }
void Invoice::setPlannedCheckOutAtSnapshot(const std::string& value) { plannedCheckOutAtSnapshot = value; }
std::string Invoice::getActualCheckInAtSnapshot() const { return actualCheckInAtSnapshot; }
void Invoice::setActualCheckInAtSnapshot(const std::string& value) { actualCheckInAtSnapshot = value; }
std::string Invoice::getActualCheckOutAtSnapshot() const { return actualCheckOutAtSnapshot; }
void Invoice::setActualCheckOutAtSnapshot(const std::string& value) { actualCheckOutAtSnapshot = value; }

bool Invoice::isValid() const {
    // Modified: Validate legacy nightly invoices and timestamp-based hourly invoices against their own immutable facts.
    const auto lockedBooking = booking.lock();

    const bool validDuration = legacyNightlyBilling
        ? nights > 0 && unitPrice > 0.0
        : actualDurationSeconds > 0 && billableHours > 0 && hourlyRoomRateSnapshot > 0.0;
    return !invoiceId.empty() &&
           lockedBooking != nullptr &&
           validDuration &&
           taxRate >= 0.0 && taxRate <= 1.0 &&
           !paymentMethod.empty() && paymentAmount > 0.0 && isIsoDateString(paymentReceivedDate) &&
           !customerNameSnapshot.empty() && !customerIdSnapshot.empty() && !customerPhoneSnapshot.empty() &&
           !roomNumberSnapshot.empty() && !roomTypeSnapshot.empty() &&
           isIsoDateString(invoiceIssuedDate) && isIsoDateString(checkInDateSnapshot) &&
           isIsoDateString(checkOutDateSnapshot);
}

double Invoice::calculateSubtotal() const {
    // Modified: New invoices bill actual elapsed time rounded half-up to whole hours; legacy records retain nightly totals.
    return legacyNightlyBilling ? nights * unitPrice : billableHours * hourlyRoomRateSnapshot;
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
    details << "<b>Price per " << (legacyNightlyBilling ? "Night" : "Hour") << ": "
            << formatMoney(legacyNightlyBilling ? unitPrice : hourlyRoomRateSnapshot) << " VND<br>";
    details << "<b>Check-in Date:</b> " << checkInDateSnapshot << "<br>";
    details << "<b>Check-out Date:</b> " << checkOutDateSnapshot << "<br>";
    if (legacyNightlyBilling) {
        details << "<b>Duration:</b> " << nights << (nights == 1 ? " night" : " nights") << "<br>";
    } else {
        details << "<b>Actual duration:</b> " << actualDurationSeconds << " seconds<br>";
        details << "<b>Billable duration:</b> " << billableHours
                << (billableHours == 1 ? " hour" : " hours") << "<br>";
    }

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
    // Modified: Display the payment fact and remaining balance separately so issuing an invoice is never confused with settlement.
    details << "<b>Payment received:</b> " << formatMoney(paymentAmount) << " VND (" << paymentMethod << ")<br>";
    details << "<b>Outstanding balance:</b> " << formatMoney(totalAmount - paymentAmount) << " VND<br>";
    details << "=========================================<br>";

    return details.str();
}

void Invoice::captureBookingSnapshot(const std::shared_ptr<Booking>& sourceBooking) {
    setBooking(sourceBooking);
    if (!sourceBooking) {
        return;
    }

    // Modified: Capture immutable guest, room, schedule, actual timestamps, and pricing facts at invoice creation.
    checkInDateSnapshot = sourceBooking->getActualCheckInDate().empty()
        ? sourceBooking->getCheckInDate() : sourceBooking->getActualCheckInDate();
    checkOutDateSnapshot = sourceBooking->getEffectiveCheckOutDate();
    unitPrice = sourceBooking->getQuotedUnitPrice();
    // Modified: Capture future datetime/hourly facts without altering the legacy invoice calculation in this migration phase.
    plannedCheckInAtSnapshot = sourceBooking->getPlannedCheckInAt();
    plannedCheckOutAtSnapshot = sourceBooking->getPlannedCheckOutAt();
    actualCheckInAtSnapshot = sourceBooking->getActualCheckInAt();
    actualCheckOutAtSnapshot = sourceBooking->getActualCheckOutAt();
    hourlyRoomRateSnapshot = sourceBooking->getQuotedHourlyRate();
    legacyNightlyBilling = sourceBooking->usesLegacyDateOnlySchedule();
    const auto customer = sourceBooking->getCustomer();
    if (customer) {
        customerNameSnapshot = customer->getName();
        customerIdSnapshot = customer->getDocumentNumber();
        customerPhoneSnapshot = customer->getPhoneNumber();
    }
    const auto room = sourceBooking->getRoom();
    if (room) {
        roomNumberSnapshot = room->getRoomNumber();
        roomTypeSnapshot = room->getRoomTypeName();
    }
}
