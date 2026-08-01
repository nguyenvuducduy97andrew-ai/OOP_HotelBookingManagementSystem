#pragma once
#include <memory>
#include <string>
#include <vector>
#include "Booking.h"
#include "BookingManager.h"
#include "Customer.h"
#include "CustomerManager.h"
#include "RoomManager.h"
#include "Invoice.h"

// HotelManager remains the stable facade for Views, ReportService, and DataManager.
// The method groups below document the current compatibility surface only;
// they do not change ownership or behavior in this phase.
std::string bookingStateToString(BookingState state);

class HotelManager
{
private:
    RoomManager m_roomManager;
    CustomerManager m_customerManager;
    BookingManager m_bookingManager;

    bool isValidDateString(
        const std::string &dateString,
        std::string &errorMessage
        ) const;

public:
    HotelManager();

    static bool isValidCustomerIdFormat(const std::string& customerId);
    static bool isValidCustomerNameFormat(const std::string& customerName);
    static bool isValidPhoneNumberFormat(const std::string& phoneNumber);

    // Read-only queries and collection accessors.
    const std::vector<std::shared_ptr<Room>>& getRooms() const;
    const std::vector<std::shared_ptr<Customer>>& getCustomers() const;
    const std::vector<std::shared_ptr<Booking>>& getBookings() const;
    const std::vector<std::shared_ptr<Invoice>>& getInvoices() const;
    const std::vector<RoomMaintenance>& getRoomMaintenances() const;
    const std::vector<MaintenanceGuestNotice>& getMaintenanceGuestNotices() const;
    std::vector<MaintenanceGuestNotice> getMaintenanceGuestNotices(const std::string& maintenanceId) const;

    // Persistence support used by DataManager during staged loads.
    void clearAll();

    // Existence checks.
    bool roomNumberExists(const std::string& roomNumber) const;
    bool customerIdExists(const std::string& customerId) const;
    bool bookingIdExists(const std::string& bookingId) const;
    bool invoiceIdExists(const std::string& invoiceId) const;

    // Entity lookups.
    std::shared_ptr<Room> findRoomByNumber(const std::string& roomNumber) const;
    std::shared_ptr<Customer> findCustomerById(const std::string& customerId) const;
    std::shared_ptr<Booking> findBookingById(const std::string& bookingId) const;
    std::shared_ptr<Invoice> findInvoiceById(const std::string& invoiceId) const;
    std::shared_ptr<Invoice> findInvoiceForBooking(const std::string& bookingId) const;

    // Read-only domain queries.
    std::vector<std::shared_ptr<Room>> getAvailableRooms() const;
    std::vector<std::shared_ptr<Room>> getAvailableRoomsForDates(
        const std::string& checkInDate,
        const std::string& checkOutDate,
        std::string& errorMessage,
        const std::string& excludedBookingId = "") const;
    bool isRoomFreeForDates(const std::string& roomNumber,
                            const std::string& checkInDate,
                            const std::string& checkOutDate,
                            std::string& errorMessage,
                            const std::string& excludedBookingId = "") const;
    std::vector<std::shared_ptr<Booking>> getBookingsForCustomer(const std::string& customerId) const;
    std::vector<std::shared_ptr<Booking>> getTodayCheckIns() const;
    std::vector<std::shared_ptr<Booking>> getTodayCheckOuts() const;
    BookingState getBookingState(const Booking &booking) const;
    std::vector<std::shared_ptr<Room>> getRoomsByOccupancy(bool occupied) const;
    bool isRoomUnderMaintenance(const std::string& roomNumber, const std::string& date) const;
    bool hasRoomMaintenanceConflict(const std::string& roomNumber, const std::string& startDate,
                                    const std::string& endDate, std::string& errorMessage) const;

    // Booking filters used by the UI and reports.
    std::vector<std::shared_ptr<Booking>> getBookingsByStatus(BookingState state) const;

    // Date-specific arrival and departure queries.
    std::vector<std::shared_ptr<Booking>> getArrivalsByDate(const std::string& dateStr) const;
    std::vector<std::shared_ptr<Booking>> getDeparturesByDate(const std::string& dateStr) const;

    // View-facing facade methods.
    bool registerRoom(
        RoomType kind,
        const std::string& roomNumber,
        double baseRate,
        std::string& errorMessage
        );

    bool registerCustomer(
        const std::string& id,
        const std::string& name,
        const std::string& phone,
        std::string& errorMessage,
        std::string* conflictingCustomerId = nullptr
        );

    // Customer edits remain behind the facade so views cannot bypass collision checks.
    bool updateCustomer(
        const std::string& customerId,
        const std::string& name,
        const std::string& phone,
        std::string& errorMessage,
        std::string* conflictingCustomerId = nullptr
        );

    bool resolveCustomerForBooking(
        const std::string& id,
        const std::string& name,
        const std::string& phone,
        std::string& errorMessage
        );

    // Booking and invoice workflow methods.
    bool createBooking(
        const std::string& customerId,
        const std::string& roomNumber,
        const std::string& checkInDate,
        const std::string& checkOutDate,
        int adultCount,
        int childCount,
        std::string& errorMessage
        );

    bool updateBooking(
        const std::string& bookingId,
        const std::string& customerId,
        const std::string& roomNumber,
        const std::string& checkInDate,
        const std::string& checkOutDate,
        int adultCount,
        int childCount,
        std::string& errorMessage
        );

    // Modified: Invoice values are derived from the booking's locked rate, tax, and actual stay duration.
    bool createInvoice(
        const std::string& invoiceId,
        const std::string& bookingId,
        const std::string& invoiceIssuedDate,
        const std::string& paymentMethod,
        double paymentAmount,
        const std::string& paymentReceivedDate,
        std::string& errorMessage
        );

    bool completeBooking(
        const std::string& bookingId,
        const std::string& checkoutDate,
        std::string& errorMessage
        );

    // Legacy permanent availability switch. New maintenance should use a dated interval below.
    bool setRoomAvailability(
        const std::string& roomNumber,
        bool available,
        std::string& errorMessage
        );

    // Maintenance and lifecycle workflow methods.
    bool checkInBooking(const std::string& bookingId, const std::string& checkInDate,
                        std::string& errorMessage);
    bool scheduleRoomMaintenance(const std::string& roomNumber, const std::string& startDate,
                                 const std::string& endDate, const std::string& note,
                                 std::string& errorMessage);
    bool cancelRoomMaintenance(const std::string& maintenanceId, std::string& errorMessage);
    bool confirmRoomMaintenance(const std::string& maintenanceId, std::string& errorMessage);

    bool archiveRoom(const std::string& roomNumber, std::string& errorMessage);
    bool archiveCustomer(const std::string& customerId, std::string& errorMessage);
    bool restoreRoom(const std::string& roomNumber, std::string& errorMessage);
    bool restoreCustomer(const std::string& customerId, std::string& errorMessage);

    // DataManager persistence restoration entry points.
    bool restoreBookingFromDatabase(
        const std::string& bookingId,
        const std::string& customerId,
        const std::string& roomNumber,
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
        std::string& errorMessage
    );
    bool restoreCustomerFromDatabase(
        const std::string& customerId,
        const std::string& documentType,
        const std::string& issuingCountry,
        const std::string& documentNumber,
        const std::string& name,
        const std::string& phone,
        bool archived,
        std::string& errorMessage
    );
    bool restoreInvoiceFromDatabase(
        const std::string& invoiceId,
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
        std::string& errorMessage
    );
    bool restoreRoomMaintenanceFromDatabase(const std::string& maintenanceId, const std::string& roomNumber,
                                            const std::string& startDate, const std::string& endDate,
                                            const std::string& note, const std::string& status,
                                            const std::string& createdAt, std::string& errorMessage);
    bool restoreMaintenanceGuestNoticeFromDatabase(const std::string& noticeId, const std::string& maintenanceId,
                                                   const std::string& bookingId, const std::string& channel,
                                                   const std::string& status, const std::string& loggedAt,
                                                   std::string& errorMessage);
    // Booking lifecycle mutations.
    bool cancelBooking(const std::string& bookingId, const std::string& reason, std::string& errorMessage);
    bool markNoShow(const std::string& bookingId, const std::string& reason, std::string& errorMessage);

    // ID generation.
    std::string nextInvoiceId() const;

    // Delete operations.
    bool deleteRoom(const std::string& roomNumber, std::string& errorMessage);
    bool deleteCustomer(const std::string& customerId, std::string& errorMessage);
    bool soft_deleteBooking(const std::string& bookingId, std::string& errorMessage);
    bool deleteInvoice(const std::string& invoiceId, std::string& errorMessage);
};
