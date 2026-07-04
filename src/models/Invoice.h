#pragma once
#include <string>
#include <memory>
#include "Booking.h"

class Booking;

class Invoice
{
private:
    std::string invoiceId;
    std::weak_ptr<Booking> booking;  // Weak reference to booking (owned by HotelManager)
    double taxRate;
    int nights;                      // Number of nights stayed (passed down from MainWindow)
    std::string paymentDate;         // ISO date format string "YYYY-MM-DD"
    bool cancelled = false;

public:
    Invoice();

    std::string getInvoiceId() const;
    void setInvoiceId(const std::string& invoiceId);

    std::shared_ptr<Booking> getBooking() const;
    void setBooking(const std::shared_ptr<Booking>& booking);
    void setBooking(Booking* booking);  // Legacy overload for raw pointers

    double getTaxRate() const;
    void setTaxRate(double taxRate);

    // Getters & Setters for the number of nights stayed
    int getNights() const;
    void setNights(int nights);

    std::string getPaymentDate() const;
    void setPaymentDate(const std::string& paymentDate);

    void setCancelled(bool c);
    bool isCancelled() const;

    // Modified: Changed return type from QString to std::string to keep Core decoupled from Qt
    std::string generateInvoiceDetails() const;

    bool isValid() const;  // Check if invoice has valid (non-expired) booking
    double calculateSubtotal() const;  // Calculate subtotal without tax

    // Calculate total amount including tax based on the duration in nights
    double calculateTotal() const;
};