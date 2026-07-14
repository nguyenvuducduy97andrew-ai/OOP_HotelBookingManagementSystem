#include <cassert>
#include <iostream>
#include "HotelManager.h"

int main()
{
    HotelManager manager;
    std::string error;

    bool roomOk = manager.registerRoom(RoomType::Standard, "101", 100.0, error);
    assert(roomOk && error.empty());

    bool customerOk = manager.registerCustomer("C1", "Alice", "0900", error);
    assert(customerOk && error.empty());

    bool bookingOk = manager.createBooking("C1", "101", "2026-07-15", "2026-07-16", error);
    assert(bookingOk && error.empty());

    auto booking = manager.findBookingById("BK1001");
    assert(booking != nullptr);

    bool invoiceOk = manager.createInvoice("INV1001", booking->getBookingId(), 0.1, 1, "2026-07-16", error);
    assert(invoiceOk && error.empty());

    auto invoice = manager.findInvoiceById("INV1001");
    assert(invoice != nullptr);

    std::cout << "Basic hotel workflow tests passed." << std::endl;
    return 0;
}
