#ifndef HOTELMANAGER_H
#define HOTELMANAGER_H

#include <memory>
#include <string>
#include <vector>

#include "Booking.h"
#include "Customer.h"
#include "Invoice.h"
#include "Room.h"

enum class RoomKind {
    Standard,
    Deluxe,
    Suite
};

class HotelManager {
public:
    HotelManager();

    static std::shared_ptr<Room> createRoom(RoomKind kind, int roomNumber, double baseRate = 0.0);

    void addRoom(std::shared_ptr<Room> room);
    const std::vector<std::shared_ptr<Room>>& getRooms() const;

    void addCustomer(std::shared_ptr<Customer> customer);
    const std::vector<std::shared_ptr<Customer>>& getCustomers() const;

    void addBooking(std::shared_ptr<Booking> booking);
    const std::vector<std::shared_ptr<Booking>>& getBookings() const;

    bool roomNumberExists(int roomNumber) const;
    bool customerIdExists(const std::string& customerId) const;
    bool bookingIdExists(const std::string& bookingId) const;

    std::shared_ptr<Room> findRoomByNumber(int roomNumber) const;
    std::shared_ptr<Customer> findCustomerById(const std::string& customerId) const;
    std::shared_ptr<Booking> findBookingById(const std::string& bookingId) const;

    std::vector<std::shared_ptr<Room>> getAvailableRooms() const;

    std::string nextBookingId() const;
    std::string nextInvoiceId() const;

    bool addRoomIfValid(RoomKind kind, int roomNumber, double baseRate, std::string& errorMessage);
    bool addCustomerIfValid(const std::string& id, const std::string& name, const std::string& phone,
                            std::string& errorMessage);
    bool addBookingIfValid(const std::string& bookingId, const std::string& customerId, int roomNumber,
                           const std::string& checkInDate, const std::string& checkOutDate,
                           std::string& errorMessage);
    bool setRoomAvailability(int roomNumber, bool available, std::string& errorMessage);

    std::shared_ptr<Invoice> createInvoice(const std::string& invoiceId, const std::string& bookingId, int days,
                                           double taxRate, std::string& errorMessage) const;

private:
    std::vector<std::shared_ptr<Room>> rooms;
    std::vector<std::shared_ptr<Customer>> customers;
    std::vector<std::shared_ptr<Booking>> bookings;
};

#endif // HOTELMANAGER_H
