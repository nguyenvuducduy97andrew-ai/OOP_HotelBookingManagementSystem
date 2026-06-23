#pragma once
#include <string>
#include <memory>
#include <QDate>
#include <QString>

class Booking;

class Invoice
{
private:
    std::string invoiceId;
    std::weak_ptr<Booking> booking;  // Weak reference to booking (owned by HotelManager)
    double totalAmount;
    double taxRate;
    QDate paymentDate;

public:
    Invoice();

    std::string getInvoiceId() const;
    void setInvoiceId(const std::string& invoiceId);

    std::shared_ptr<Booking> getBooking() const;
    void setBooking(const std::shared_ptr<Booking>& booking);
    void setBooking(Booking* booking);  // Legacy overload for raw pointers

    double getTotalAmount() const;
    void setTotalAmount(double totalAmount);

    double getTaxRate() const;
    void setTaxRate(double taxRate);

    QDate getPaymentDate() const;
    void setPaymentDate(const QDate& paymentDate);

    QString generateInvoiceDetails();
    bool isValid() const;  // Check if invoice has valid (non-expired) booking
    double calculateSubtotal() const;  // Calculate subtotal without tax
};

