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