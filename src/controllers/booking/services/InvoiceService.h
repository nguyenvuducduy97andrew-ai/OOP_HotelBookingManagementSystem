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
        const std::string& invoiceIssuedDate,
        std::string& errorMessage);

private:
    HotelManager& m_hotelManager;
};
