#pragma once
#include <memory>
#include <string>
#include <vector>
#include "Booking.h"
#include "Customer.h"
#include "Invoice.h"
#include "Room.h"
#include "RoomFactory.h"
enum class RoomKind
{
    Standard,
    Deluxe,
    Suite
};

class HotelManager
{
private:
    std::vector<std::shared_ptr<Room>> rooms;
    std::vector<std::shared_ptr<Customer>> customers;
    std::vector<std::shared_ptr<Booking>> bookings;

public:
    HotelManager();

    static std::shared_ptr<Room> createRoom(RoomKind kind, int roomNumber, double baseRate = 0.0);//static là để gọi trực tiếp, không cần tạo object HotelManager

    void addRoom(std::shared_ptr<Room> room);//thêm phòng
    const std::vector<std::shared_ptr<Room>> &getRooms() const;//truy xuất phòng, reference để không tạo copy, const để không cho phép thay đổi dữ liệu

    void addCustomer(std::shared_ptr<Customer> customer);//thêm khách hàng
    const std::vector<std::shared_ptr<Customer>> &getCustomers() const;//truy xuất khách hàng

    void addBooking(std::shared_ptr<Booking> booking);//thêm dữ liệu book
    const std::vector<std::shared_ptr<Booking>> &getBookings() const;//truy xuất dữ diệu book
//Kiểm tra những object room, customer hay booking đã tồn tại hay chưa
    bool roomNumberExists(int roomNumber) const;
    bool customerIdExists(const std::string &customerId) const;
    bool bookingIdExists(const std::string &bookingId) const;
//Tìm các object theo ID/number
    std::shared_ptr<Room> findRoomByNumber(int roomNumber) const;
    std::shared_ptr<Customer> findCustomerById(const std::string &customerId) const;
    std::shared_ptr<Booking> findBookingById(const std::string &bookingId) const;
//Trả về danh sách những phòng trống
    std::vector<std::shared_ptr<Room>> getAvailableRooms() const;
//Sinh thêm Booking ID và Invoice ID
    std::string nextBookingId() const;
    std::string nextInvoiceId() const;

    bool addRoomIfValid(RoomKind kind, int roomNumber, double baseRate, std::string &errorMessage);
    bool addCustomerIfValid(const std::string &id, const std::string &name, const std::string &phone,
                            std::string &errorMessage);
    bool addBookingIfValid(const std::string &bookingId, const std::string &customerId, int roomNumber,
                           const std::string &checkInDate, const std::string &checkOutDate,
                           std::string &errorMessage);
    bool setRoomAvailability(int roomNumber, bool available, std::string &errorMessage);

    std::shared_ptr<Invoice> createInvoice(const std::string &invoiceId, const std::string &bookingId, int days,
                                           double taxRate, std::string &errorMessage) const;
};
