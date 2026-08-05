#include "SuiteRoom.h"

SuiteRoom::SuiteRoom(std::string num, double basePrice, double fee)
    : Room(num, basePrice),
      premiumServiceFee(fee) {
    this->setArea(55.0);
    this->setBedType("Super King Bed");
    this->setMaxGuests(4);
    this->setDescription("Luxury suite with premium services and separate living area.");
    this->setAmenities("Free Wi-Fi, Air Conditioning, 2 Smart TVs, Mini Bar, Bathtub, Premium Service");
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
    return this->getMaxGuests();
}

double SuiteRoom::getExtraFeeAmount() const {
    // Fixed-modified: Surface the premium service fee through the base room interface.
    return getPremiumServiceFee();
}

void SuiteRoom::setExtraFeeAmount(double fee) {
    setPremiumServiceFee(fee);
}
