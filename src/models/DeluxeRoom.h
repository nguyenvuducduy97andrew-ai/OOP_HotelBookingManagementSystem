#pragma once
#include "Room.h"
#include <string>

class DeluxeRoom : public Room {
private:
    double miniBarFee; // Đổi từ Deluxe_Fee thành miniBarFee theo đúng sơ đồ lớp

public:
    // Hàm khởi tạo constructor đồng bộ tên tham số
    DeluxeRoom(std::string num, double basePrice, double fee);

    // GET/SET cho miniBarFee (đổi từ Get_Deluxe_Fee / Set_Deluxe_Fee)
    double getMiniBarFee() const;
    void setMiniBarFee(double fee);

    // Override function calculateTargetPrice từ lớp cha Room
    double calculateTargetPrice() const override;
    std::string getRoomTypeName() const override;
    double getExtraFeeAmount() const override;
    void setExtraFeeAmount(double fee) override;
};