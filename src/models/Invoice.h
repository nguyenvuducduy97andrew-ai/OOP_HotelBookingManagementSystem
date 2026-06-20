#pragma once
#include <string>
#include <QDate>
#include <QString>

class Booking;

class Invoice
{
private:
    std::string invoiceId;
    Booking* booking;
    double totalAmount;
    QDate paymentDate;

public:
    Invoice();

    std::string getInvoiceId() const;
    void setInvoiceId(const std::string& invoiceId);

    Booking* getBooking() const;
    void setBooking(Booking* booking);

    double getTotalAmount() const;
    void setTotalAmount(double totalAmount);

    QDate getPaymentDate() const;
    void setPaymentDate(const QDate& paymentDate);

    QString generateInvoiceDetails();
};

