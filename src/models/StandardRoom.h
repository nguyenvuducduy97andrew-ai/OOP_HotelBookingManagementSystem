#pragma once
#include "Room.h"
#include <string>

class StandardRoom : public Room {
public:
    StandardRoom(std::string num, double basePrice);

    double calculateTargetPrice() const override;
};