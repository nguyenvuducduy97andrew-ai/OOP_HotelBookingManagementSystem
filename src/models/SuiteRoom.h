#pragma once
#include "Room.h"
#include <string>

class SuiteRoom : public Room {
private:
    double premiumServiceFee;

public:
    SuiteRoom(std::string num, double basePrice, double fee);

    double getPremiumServiceFee() const;
    void setPremiumServiceFee(double fee);

    double calculateTargetPrice() override;
};