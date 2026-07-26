#include "StandardRoom.h"
#include <string>

StandardRoom::StandardRoom(std::string num, double basePrice)
    : Room(num, basePrice) {
}

double StandardRoom::calculateTargetPrice() const {
    return this->getBasePrice();
}