#include "StandardRoom.h"
#include <string>

StandardRoom::StandardRoom(std::string num, double basePrice)
    : Room(num, basePrice) {
}

double StandardRoom::calculateTargetPrice() const {
    return this->getBasePrice();
}

std::string StandardRoom::getRoomTypeName() const {
    return "Standard";
}

double StandardRoom::getExtraFeeAmount() const {
    // Fixed-modified: Standard rooms have no subtype fee.
    return 0.0;
}

void StandardRoom::setExtraFeeAmount(double) {
}