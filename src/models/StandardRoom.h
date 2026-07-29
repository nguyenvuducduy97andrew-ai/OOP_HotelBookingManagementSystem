#pragma once
#include "Room.h"
#include <string>

class StandardRoom : public Room {
public:
    StandardRoom(std::string num, double basePrice);

    double calculateTargetPrice() const override;
    std::string getRoomTypeName() const override;
    int getMaximumGuests() const override;
    double getExtraFeeAmount() const override;
    void setExtraFeeAmount(double fee) override;
};
