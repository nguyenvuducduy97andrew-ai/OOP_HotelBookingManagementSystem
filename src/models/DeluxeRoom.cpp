#include "DeluxeRoom.h"
#include <string>

DeluxeRoom::DeluxeRoom(std::string num, double basePrice, double miniBarFee)
    : Room(num, basePrice),
      miniBarFee(miniBarFee) {
}

double DeluxeRoom::getMiniBarFee() const {
    return this->miniBarFee;
}

void DeluxeRoom::setMiniBarFee(double fee) {
    this->miniBarFee = fee;
}

double DeluxeRoom::calculateTargetPrice() const {
    return this->getMiniBarFee() + this->getBasePrice();
}

std::string DeluxeRoom::getRoomTypeName() const {
    return "Deluxe";
}

int DeluxeRoom::getMaximumGuests() const {
    // Modified: Define Deluxe inventory capacity centrally for reservation validation.
    return 3;
}

double DeluxeRoom::getExtraFeeAmount() const {
    // Fixed-modified: Surface the mini-bar fee through the base room interface.
    return getMiniBarFee();
}

void DeluxeRoom::setExtraFeeAmount(double fee) {
    setMiniBarFee(fee);
}
