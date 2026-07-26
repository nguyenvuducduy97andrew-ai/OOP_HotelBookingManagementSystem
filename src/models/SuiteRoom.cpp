#include "SuiteRoom.h"

SuiteRoom::SuiteRoom(std::string num, double basePrice, double fee)
    : Room(num, basePrice),
      premiumServiceFee(fee) {
}

double SuiteRoom::getPremiumServiceFee() const {
    return premiumServiceFee;
}

void SuiteRoom::setPremiumServiceFee(double fee) {
    premiumServiceFee = fee;
}

double SuiteRoom::calculateTargetPrice() const {
    return getBasePrice() + premiumServiceFee;
}

std::string SuiteRoom::getRoomTypeName() const {
    return "Suite";
}

double SuiteRoom::getExtraFeeAmount() const {
    // Fixed-modified: Surface the premium service fee through the base room interface.
    return getPremiumServiceFee();
}

void SuiteRoom::setExtraFeeAmount(double fee) {
    setPremiumServiceFee(fee);
}