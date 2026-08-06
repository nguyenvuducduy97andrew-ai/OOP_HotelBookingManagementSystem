#pragma once
#include <string>
#include <memory>
#include "Booking.h"

class Booking;
class BookingManager;

class Invoice
{
private:
    std::string invoiceId;
    std::string bookingId;
    std::weak_ptr<Booking> booking;  // Weak reference to booking (owned by HotelManager)
    double taxRate;
    int nights;                      // Number of nights stayed (passed down from MainWindow)
    // Modified: Retain future actual-duration billing facts while legacy invoices continue to use their immutable nightly totals.
    long long actualDurationSeconds;
    int billableHours;
    bool legacyNightlyBilling;
    // Modified: This is the invoice issue date, not evidence that the balance has been paid.
    std::string invoiceIssuedDate;   // ISO date format string "YYYY-MM-DD"
    std::string paymentMethod;
    double paymentAmount;
    std::string paymentReceivedDate;
    // Modified: Store immutable billing snapshots so completed invoices do not depend on mutable model records.
    double unitPrice;
    double hourlyRoomRateSnapshot;
    std::string customerNameSnapshot;
    std::string customerIdSnapshot;
    std::string customerPhoneSnapshot;
    std::string roomNumberSnapshot;
    std::string roomTypeSnapshot;
    std::string checkInDateSnapshot;
    std::string checkOutDateSnapshot;
    std::string plannedCheckInAtSnapshot;
    std::string plannedCheckOutAtSnapshot;
    std::string actualCheckInAtSnapshot;
    std::string actualCheckOutAtSnapshot;

public:
    Invoice();

    // Modified and optimized performance: restrict invoice-ID assignment to persistence restoration and the controlled invoice workflow.
    friend class BookingManager;

    std::string getInvoiceId() const;
    
private:
    // Fixed-modified: Keep invoice IDs write-protected outside the manager layer.
    void setInvoiceId(const std::string& invoiceId);

public:

    std::string getBookingId() const;
    void setBookingId(const std::string& bookingId);

    std::shared_ptr<Booking> getBooking() const;
    void setBooking(const std::shared_ptr<Booking>& booking);
    // Modified: Capture the linked booking's billable values at invoice creation time.
    void captureBookingSnapshot(const std::shared_ptr<Booking>& booking);

    double getTaxRate() const;
    void setTaxRate(double taxRate);

    // Getters & Setters for the number of nights stayed
    int getNights() const;
    void setNights(int nights);
    long long getActualDurationSeconds() const;
    void setActualDurationSeconds(long long value);
    int getBillableHours() const;
    void setBillableHours(int value);
    bool usesLegacyNightlyBilling() const;
    void setLegacyNightlyBilling(bool value);

    std::string getInvoiceIssuedDate() const;
    void setInvoiceIssuedDate(const std::string& invoiceIssuedDate);

    std::string getPaymentMethod() const;
    void setPaymentMethod(const std::string& value);
    double getPaymentAmount() const;
    void setPaymentAmount(double value);
    std::string getPaymentReceivedDate() const;
    void setPaymentReceivedDate(const std::string& value);

    double getUnitPrice() const;
    void setUnitPrice(double value);
    double getHourlyRoomRateSnapshot() const;
    void setHourlyRoomRateSnapshot(double value);
    std::string getCustomerNameSnapshot() const;
    void setCustomerNameSnapshot(const std::string& value);
    std::string getCustomerIdSnapshot() const;
    void setCustomerIdSnapshot(const std::string& value);
    std::string getCustomerPhoneSnapshot() const;
    void setCustomerPhoneSnapshot(const std::string& value);
    std::string getRoomNumberSnapshot() const;
    void setRoomNumberSnapshot(const std::string& value);
    std::string getRoomTypeSnapshot() const;
    void setRoomTypeSnapshot(const std::string& value);
    std::string getCheckInDateSnapshot() const;
    void setCheckInDateSnapshot(const std::string& value);
    std::string getCheckOutDateSnapshot() const;
    void setCheckOutDateSnapshot(const std::string& value);
    std::string getPlannedCheckInAtSnapshot() const;
    void setPlannedCheckInAtSnapshot(const std::string& value);
    std::string getPlannedCheckOutAtSnapshot() const;
    void setPlannedCheckOutAtSnapshot(const std::string& value);
    std::string getActualCheckInAtSnapshot() const;
    void setActualCheckInAtSnapshot(const std::string& value);
    std::string getActualCheckOutAtSnapshot() const;
    void setActualCheckOutAtSnapshot(const std::string& value);

    // Modified: Changed return type from QString to std::string to keep Core decoupled from Qt
    std::string generateInvoiceDetails() const;

    bool isValid() const;  // Check if invoice has valid (non-expired) booking
    double calculateSubtotal() const;  // Calculate subtotal without tax

    // Calculate total amount including tax based on the duration in nights
    double calculateTotal() const;
};
