#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Booking.h"
#include "Customer.h"
#include "Invoice.h"
#include "Room.h"
#include "RoomMaintenance.h"

class Customer;
class Room;
class RoomMaintenance;

class BookingManager
{
public:
    BookingManager() = default;

    const std::vector<std::shared_ptr<Booking>>& getBookings() const;
    std::shared_ptr<Booking> findBookingById(const std::string& bookingId) const;
    bool bookingIdExists(const std::string& bookingId) const;
    BookingState getBookingState(const Booking& booking) const;
    const std::vector<std::shared_ptr<Invoice>>& getInvoices() const;
    std::shared_ptr<Invoice> findInvoiceById(const std::string& invoiceId) const;
    std::shared_ptr<Invoice> findInvoiceForBooking(const std::string& bookingId) const;
    bool invoiceIdExists(const std::string& invoiceId) const;
    std::string nextInvoiceId() const;

    std::vector<std::shared_ptr<Booking>> getBookingsForCustomer(const std::string& customerId) const;
    std::vector<std::shared_ptr<Booking>> getBookingsByStatus(BookingState state) const;
    std::vector<std::shared_ptr<Booking>> getArrivalsByDate(const std::string& dateStr) const;
    std::vector<std::shared_ptr<Booking>> getDeparturesByDate(const std::string& dateStr) const;

    bool hasBookingForRoom(const std::string& roomNumber) const;
    bool hasBookingForCustomer(const std::string& customerId) const;

    bool isRoomFreeForDates(const std::string& roomNumber,
                            const std::string& checkInDate,
                            const std::string& checkOutDate,
                            const std::vector<RoomMaintenance>& roomMaintenances,
                            std::string& errorMessage,
                            const std::string& excludedBookingId = "") const;
    std::vector<std::shared_ptr<Room>> getAvailableRoomsForDates(
        const std::string& checkInDate,
        const std::string& checkOutDate,
        const std::vector<std::shared_ptr<Room>>& rooms,
        const std::vector<RoomMaintenance>& roomMaintenances,
        std::string& errorMessage,
        const std::string& excludedBookingId = "") const;

    bool createBooking(const std::string& customerId,
                       const std::string& roomNumber,
                       const std::string& checkInDate,
                       const std::string& checkOutDate,
                       int adultCount,
                       int childCount,
                       std::string& errorMessage);
    bool createBookingResolved(const std::shared_ptr<Customer>& customer,
                               const std::shared_ptr<Room>& room,
                               const std::string& checkInDate,
                               const std::string& checkOutDate,
                               int adultCount,
                               int childCount,
                               const std::vector<RoomMaintenance>& roomMaintenances,
                               std::string& errorMessage);

    bool updateBooking(const std::string& bookingId,
                       const std::string& customerId,
                       const std::string& roomNumber,
                       const std::string& checkInDate,
                       const std::string& checkOutDate,
                       int adultCount,
                       int childCount,
                       std::string& errorMessage);
    bool updateBookingResolved(const std::string& bookingId,
                               const std::string& customerId,
                               const std::string& roomNumber,
                               const std::shared_ptr<Customer>& customer,
                               const std::shared_ptr<Room>& room,
                               const std::string& checkInDate,
                               const std::string& checkOutDate,
                               int adultCount,
                               int childCount,
                               const std::vector<RoomMaintenance>& roomMaintenances,
                               std::string& errorMessage);

    bool checkInBooking(const std::string& bookingId, const std::string& checkInDate,
                        std::string& errorMessage);
    bool cancelBooking(const std::string& bookingId, const std::string& reason, std::string& errorMessage);
    bool markNoShow(const std::string& bookingId, const std::string& reason, std::string& errorMessage);
    bool completeBooking(const std::string& bookingId, const std::string& checkoutDate,
                         std::string& errorMessage);

    bool createInvoice(const std::string& invoiceId,
                       const std::string& bookingId,
                       const std::string& invoiceIssuedDate,
                       const std::string& paymentMethod,
                       double paymentAmount,
                       const std::string& paymentReceivedDate,
                       std::string& errorMessage);

    bool restoreBookingFromDatabase(const std::string& bookingId,
                                    const std::shared_ptr<Customer>& customer,
                                    const std::shared_ptr<Room>& room,
                                    const std::string& checkInDate,
                                    const std::string& checkOutDate,
                                    bool cancelled,
                                    bool deleted,
                                    bool checkedIn,
                                    bool checkedOut,
                                    const std::string& actualCheckInDate,
                                    const std::string& actualCheckOutDate,
                                    double quotedUnitPrice,
                                    double quotedTaxRate,
                                    int adultCount,
                                    int childCount,
                                    const std::string& cancellationReason,
                                    const std::string& cancelledAt,
                                    const std::string& createdAt,
                                    const std::string& updatedAt,
                                    std::string& errorMessage);
    bool restoreInvoiceFromDatabase(const std::string& invoiceId,
                                    const std::string& bookingId,
                                    double taxRate,
                                    int nights,
                                    const std::string& invoiceIssuedDate,
                                    const std::string& paymentMethod,
                                    double paymentAmount,
                                    const std::string& paymentReceivedDate,
                                    double unitPrice,
                                    const std::string& customerNameSnapshot,
                                    const std::string& customerIdSnapshot,
                                    const std::string& customerPhoneSnapshot,
                                    const std::string& roomNumberSnapshot,
                                    const std::string& roomTypeSnapshot,
                                    const std::string& checkInDateSnapshot,
                                    const std::string& checkOutDateSnapshot,
                                    std::string& errorMessage);

    void clearAll();

private:
    bool validateBookingDates(const std::string& checkInDate,
                              const std::string& checkOutDate,
                              std::string& errorMessage) const;
    bool validateBookingInput(const std::shared_ptr<Customer>& customer,
                              const std::shared_ptr<Room>& room,
                              const std::string& checkInDate,
                              const std::string& checkOutDate,
                              int adultCount,
                              int childCount,
                              const std::vector<RoomMaintenance>& roomMaintenances,
                              std::string& errorMessage) const;
    std::vector<std::shared_ptr<Booking>> m_bookings;
    std::vector<std::shared_ptr<Invoice>> m_invoices;
};
