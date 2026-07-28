#pragma once
#include <memory>
#include <string>
#include <vector>
#include "Booking.h"
#include "Customer.h"
#include "Invoice.h"
#include "Room.h"
#include "RoomFactory.h"
#include "RoomMaintenance.h"

class BookingService;
class InvoiceService;
class BookingManager;
class CustomerService;
class RoomService;

// Fixed-modified: Move booking state ownership closer to the booking model while keeping manager queries intact.
std::string bookingStateToString(BookingState state);

class HotelManager
{
private:
    // Modified and optimized performance: allow only focused workflow services to append validated domain objects to the manager-owned collections.
    friend class BookingService;
    friend class InvoiceService;
    friend class CustomerService;
    friend class RoomService;

    std::vector<std::shared_ptr<Room>> rooms;
    std::vector<std::shared_ptr<Customer>> customers;
    std::vector<std::shared_ptr<Booking>> bookings;
    std::vector<std::shared_ptr<Invoice>> invoices;
    std::vector<RoomMaintenance> roomMaintenances;

    // Validation helpers
    bool isValidRoomNumber(const std::string& roomNumber) const;

    bool validateRoomInput(
        const std::string& roomNumber,
        double baseRate,
        std::string& errorMessage
        ) const;

    bool validateCustomerInput(
        const std::string& id,
        const std::string& name,
        const std::string& phone,
        std::string& errorMessage
        ) const;
    bool isValidDateString(
        const std::string &dateString,
        std::string &errorMessage
        ) const;

    void addRoom(std::shared_ptr<Room> room);
    void addCustomer(std::shared_ptr<Customer> customer);
    void addBooking(std::shared_ptr<Booking> booking);
    void addInvoice(std::shared_ptr<Invoice> invoice);

    bool registerRoomCore(RoomType kind, const std::string& roomNumber, double baseRate, std::string& errorMessage);
    bool registerCustomerCore(const std::string& id, const std::string& name, const std::string& phone, std::string& errorMessage);
    // Modified: Centralize validation for mutable customer fields while keeping the customer ID immutable.
    bool updateCustomerCore(const std::string& customerId, const std::string& name, const std::string& phone, std::string& errorMessage);
    bool resolveCustomerForBookingCore(const std::string& id, const std::string& name, const std::string& phone, std::string& errorMessage);
    bool setRoomAvailabilityCore(const std::string& roomNumber, bool available, std::string& errorMessage);
    bool scheduleRoomMaintenanceCore(const std::string& roomNumber, const std::string& startDate,
                                     const std::string& endDate, const std::string& note,
                                     std::string& errorMessage);
    bool cancelRoomMaintenanceCore(const std::string& maintenanceId, std::string& errorMessage);
    bool archiveRoomCore(const std::string& roomNumber, std::string& errorMessage);
    bool archiveCustomerCore(const std::string& customerId, std::string& errorMessage);
    bool restoreRoomCore(const std::string& roomNumber, std::string& errorMessage);
    bool restoreCustomerCore(const std::string& customerId, std::string& errorMessage);
    bool deleteRoomCore(const std::string& roomNumber, std::string& errorMessage);
    bool deleteCustomerCore(const std::string& customerId, std::string& errorMessage);

public:
    HotelManager();

    static bool isValidCustomerIdFormat(const std::string& customerId);
    static bool isValidCustomerNameFormat(const std::string& customerName);
    static bool isValidPhoneNumberFormat(const std::string& phoneNumber);

    // Getters
    const std::vector<std::shared_ptr<Room>>& getRooms() const;
    const std::vector<std::shared_ptr<Customer>>& getCustomers() const;
    const std::vector<std::shared_ptr<Booking>>& getBookings() const;
    const std::vector<std::shared_ptr<Invoice>>& getInvoices() const;
    const std::vector<RoomMaintenance>& getRoomMaintenances() const;

    // Modified: Moved internal add methods to public so DataManager can populate entities when loading database
    
    void clearAll();

    // Existence checks
    bool roomNumberExists(const std::string& roomNumber) const;
    bool customerIdExists(const std::string& customerId) const;
    bool bookingIdExists(const std::string& bookingId) const;
    bool invoiceIdExists(const std::string& invoiceId) const;

    // Find methods
    std::shared_ptr<Room> findRoomByNumber(const std::string& roomNumber) const;
    std::shared_ptr<Customer> findCustomerById(const std::string& customerId) const;
    std::shared_ptr<Booking> findBookingById(const std::string& bookingId) const;
    std::shared_ptr<Invoice> findInvoiceById(const std::string& invoiceId) const;
    std::shared_ptr<Invoice> findInvoiceForBooking(const std::string& bookingId) const;

    // Queries
    std::vector<std::shared_ptr<Room>> getAvailableRooms() const;
    std::vector<std::shared_ptr<Booking>> getBookingsForCustomer(const std::string& customerId) const;
    std::vector<std::shared_ptr<Booking>> getTodayCheckIns() const;
    std::vector<std::shared_ptr<Booking>> getTodayCheckOuts() const;
    BookingState getBookingState(const Booking &booking) const;
    std::vector<std::shared_ptr<Room>> getRoomsByOccupancy(bool occupied) const;
    bool isRoomUnderMaintenance(const std::string& roomNumber, const std::string& date) const;
    bool hasRoomMaintenanceConflict(const std::string& roomNumber, const std::string& startDate,
                                    const std::string& endDate, std::string& errorMessage) const;

    // Added: Filter bookings by their specific operational status for tabbed interface sub-pages
    std::vector<std::shared_ptr<Booking>> getBookingsByStatus(BookingState state) const;

    // Added: Fetch today's dynamic arrivals and departures targeting a custom date string for the Dashboard
    std::vector<std::shared_ptr<Booking>> getArrivalsByDate(const std::string& dateStr) const;
    std::vector<std::shared_ptr<Booking>> getDeparturesByDate(const std::string& dateStr) const;

    // Use-case facade methods. Their business rules live in the dedicated services.
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

    // Modified: Delegate customer edits to CustomerService so views cannot bypass collision checks.
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

    bool createBooking(
        const std::string& customerId,
        const std::string& roomNumber,
        const std::string& checkInDate,
        const std::string& checkOutDate,
        std::string& errorMessage
        );

    bool updateBooking(
        const std::string& bookingId,
        const std::string& customerId,
        const std::string& roomNumber,
        const std::string& checkInDate,
        const std::string& checkOutDate,
        std::string& errorMessage
        );

    // Modified: Added 'nights' and 'paymentDate' so the view layer can pass down computed duration and billing timestamps
    bool createInvoice(
        const std::string& invoiceId,
        const std::string& bookingId,
        double taxRate,
        int nights,
        const std::string& paymentDate,
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
    bool scheduleRoomMaintenance(const std::string& roomNumber, const std::string& startDate,
                                 const std::string& endDate, const std::string& note,
                                 std::string& errorMessage);
    bool cancelRoomMaintenance(const std::string& maintenanceId, std::string& errorMessage);

    bool archiveRoom(const std::string& roomNumber, std::string& errorMessage);
    bool archiveCustomer(const std::string& customerId, std::string& errorMessage);
    bool restoreRoom(const std::string& roomNumber, std::string& errorMessage);
    bool restoreCustomer(const std::string& customerId, std::string& errorMessage);

    //Data Manager integration methods for persistence, only used by DataManager to restore objects from database
    bool restoreBookingFromDatabase(
        const std::string& bookingId,
        const std::string& customerId,
        const std::string& roomNumber,
        const std::string& checkInDate,
        const std::string& checkOutDate,
        bool cancelled,
        bool deleted,
        bool checkedOut,
        std::string& errorMessage
    );
    bool restoreCustomerFromDatabase(
        const std::string& customerId,
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
        const std::string& paymentDate,
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
                                            const std::string& note, std::string& errorMessage);
    // Added: Soft-cancels a reservation by changing its status and releasing the assigned room back to inventory
    bool cancelBooking(const std::string& bookingId, std::string& errorMessage);

    // ID generation
    std::string nextInvoiceId() const;

    // Fixed-modified: Make booking deletion consistent with the other hard-delete operations.
    // Delete methods
    bool deleteRoom(const std::string& roomNumber, std::string& errorMessage);
    bool deleteCustomer(const std::string& customerId, std::string& errorMessage);
    bool soft_deleteBooking(const std::string& bookingId, std::string& errorMessage);
    bool deleteInvoice(const std::string& invoiceId, std::string& errorMessage);
};
