#include "Invoice.h"
#include "Booking.h"
#include "Room.h"
#include "Customer.h"
#include <iostream>
#include <iomanip>
using namespace std;

Invoice::Invoice() {
    this->invoiceId = "INV_UNKNOWN";
    this->booking = nullptr;
    this->totalAmount = 0.0;
    this->paymentDate = QDate::currentDate();
}

string Invoice::getInvoiceId() const {
    return invoiceId;
}
void Invoice::setInvoiceId(const string& invoiceId) {
    this->invoiceId = invoiceId;
}

Booking* Invoice::getBooking() const {
    return booking;
}
void Invoice::setBooking(Booking* booking) {
    this->booking = booking;
    if (booking!=nullptr && booking->getRoom()!=nullptr) {
        int nights = booking->getDurationInNights();
        double roomPrice = booking->getRoom()->getBasePrice();

        this->totalAmount = nights*roomPrice;
    }
    else {
        this->totalAmount = 0.0;
    }
}

double Invoice::getTotalAmount() const {
    return totalAmount;
}
void Invoice::setTotalAmount(double totalAmount) {
    this->totalAmount = totalAmount;
}

QDate Invoice::getPaymentDate() const {
    return paymentDate;
}
void Invoice::setPaymentDate(const QDate& paymentDate) {
    this->paymentDate = paymentDate;
}

QString Invoice::generateInvoiceDetails() {
    QString details;

    details += "<h3>====== HOTEL BOOKING INVOICE ======</h3>";
    details += "<b>Invoice ID:</b> " + QString::fromStdString(invoiceId) + "<br>";
    details += "<b>Payment Date:</b> " + paymentDate.toString("dd/MM/yyyy") + "<br>";
    details += "--------------------------------------------------<br>";

    if (booking == nullptr) {
        details += "<font color='red'><b>Error: No booking data associated!</b></font><br>";
        details += "=========================================<br>";
        return details;
    }

    if (booking->getCustomer()!=nullptr) {
        details += "<b>Customer Name:</b> " + QString::fromStdString(booking->getCustomer()->getName()) + "<br>";
    }
    else {
        details += "<b>Customer Name:</b> Unknown<br>";
    }

    if (booking->getRoom() != nullptr) {
        details += "<b>Room Number:</b> " + QString::fromStdString(booking->getRoom()->getRoomNumber()) + "<br>";
        details += "<b>Room Price per Night:</b> $" + QString::number(booking->getRoom()->getBasePrice(), 'f', 2) + "<br>";
    } else {
        details += "<b>Room Info:</b> Not Assigned<br>";
    }

    details += "<b>Check-in Date:</b> " + booking->getCheckInDate().toString("dd/MM/yyyy") + "<br>";
    details += "<b>Check-out Date:</b> " + booking->getCheckOutDate().toString("dd/MM/yyyy") + "<br>";
    details += "<b>Duration:</b> " + QString::number(booking->getDurationInNights()) + " night(s)<br>";

    details += "--------------------------------------------------<br>";
    details += "<h2>TOTAL AMOUNT: $" + QString::number(totalAmount, 'f', 2) + "</h2>";
    details += "========================================= <br>";

    return details;
}