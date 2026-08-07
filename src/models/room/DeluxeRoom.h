#pragma once
#include "Room.h"
#include <string>

class DeluxeRoom : public Room {
private:
    double miniBarFee; // Renamed from Deluxe_Fee to match the class diagram.

public:
    // Constructor parameter names are aligned with the model.
    DeluxeRoom(std::string num, double basePrice, double fee);

    // Accessors for miniBarFee (renamed from Get/Set_Deluxe_Fee).
    double getMiniBarFee() const;
    void setMiniBarFee(double fee);

    // Override calculateTargetPrice from the Room base class.
    double calculateTargetPrice() const override;
    std::string getRoomTypeName() const override;
    int getMaximumGuests() const override;
    double getExtraFeeAmount() const override;
    void setExtraFeeAmount(double fee) override;
};
