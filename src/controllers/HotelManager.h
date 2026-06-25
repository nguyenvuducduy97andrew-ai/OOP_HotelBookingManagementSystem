#pragma once
#include <memory>
#include <string>
#include <vector>
#include "Booking.h"
#include "Customer.h"
#include "Invoice.h"
#include "Room.h"
#include "RoomFactory.h"


class HotelManager
{
private:
    std::vector<std::shared_ptr<Room>> rooms;
    std::vector<std::shared_ptr<Customer>> customers;
    std::vector<std::shared_ptr<Booking>> bookings;
    std::vector<std::shared_ptr<Invoice>> invoices;

    // Internal add methods
    void addRoom(std::shared_ptr<Room> room);
    void addCustomer(std::shared_ptr<Customer> customer);
    void addBooking(std::shared_ptr<Booking> booking);
    void addInvoice(std::shared_ptr<Invoice> invoice);
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

    bool validateBookingInput(
        const std::string& customerId,
        const std::string& roomNumber,
        const std::string& checkInDate,
        const std::string& checkOutDate,
        std::string& errorMessage
    ) const;
    bool validateInvoiceInput(
        const std::string& invoiceId,
        const std::string& bookingId,
        int days,
        double taxRate,
        std::string& errorMessage
    ) const;

public:
    HotelManager();

    // Getters
    const std::vector<std::shared_ptr<Room>>& getRooms() const;
    const std::vector<std::shared_ptr<Customer>>& getCustomers() const;
    const std::vector<std::shared_ptr<Booking>>& getBookings() const;
    const std::vector<std::shared_ptr<Invoice>>& getInvoices() const;

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
    // Queries
    std::vector<std::shared_ptr<Room>> getAvailableRooms() const;

    // Use-case methods
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
        std::string& errorMessage
    );

    bool createBooking(
        const std::string& customerId,
        const std::string& roomNumber,
        const std::string& checkInDate,
        const std::string& checkOutDate,
        std::string& errorMessage
    );

    bool setRoomAvailability(
        const std::string& roomNumber,
        bool available,
        std::string& errorMessage
    );

    // ID generation
    std::string nextInvoiceId() const;

    // Invoice
    std::shared_ptr<Invoice> createInvoice(
        const std::string& invoiceId,
        const std::string& bookingId,
        int days,
        double taxRate,
        std::string& errorMessage
    );
    // Delete methods
    bool deleteRoom(const std::string& roomNumber, std::string& errorMessage);
    bool deleteCustomer(const std::string& customerId, std::string& errorMessage);
    bool deleteBooking(const std::string& bookingId, std::string& errorMessage);
    bool deleteInvoice(const std::string& invoiceId, std::string& errorMessage);
};