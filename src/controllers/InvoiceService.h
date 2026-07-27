#pragma once

#include <string>

class HotelManager;

class InvoiceService
{
public:
    explicit InvoiceService(HotelManager& hotelManager);

    bool createInvoice(
        const std::string& invoiceId,
        const std::string& bookingId,
        double taxRate,
        int nights,
        const std::string& paymentDate,
        std::string& errorMessage);

private:
    HotelManager& m_hotelManager;
};
