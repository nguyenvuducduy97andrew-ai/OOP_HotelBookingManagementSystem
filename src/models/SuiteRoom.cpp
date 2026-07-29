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

int SuiteRoom::getMaximumGuests() const {
    // Modified: Define Suite inventory capacity centrally for reservation validation.
    return 4;
}

double SuiteRoom::getExtraFeeAmount() const {
    // Fixed-modified: Surface the premium service fee through the base room interface.
    return getPremiumServiceFee();
}

void SuiteRoom::setExtraFeeAmount(double fee) {
    setPremiumServiceFee(fee);
}
