#include "StandardRoom.h"
#include <string>

StandardRoom::StandardRoom(std::string num, double basePrice)
    : Room(num, basePrice) {
    this->setArea(25.0);
    this->setBedType("Queen Bed");
    this->setMaxGuests(2);
    this->setDescription("Standard room with basic amenities.");
    this->setAmenities("Free Wi-Fi, Air Conditioning, TV");
}

double StandardRoom::calculateTargetPrice() const {
    return this->getBasePrice();
}

std::string StandardRoom::getRoomTypeName() const {
    return "Standard";
}

int StandardRoom::getMaximumGuests() const {
    return this->getMaxGuests();
}

double StandardRoom::getExtraFeeAmount() const {
    // Fixed-modified: Standard rooms have no subtype fee.
    return 0.0;
}

void StandardRoom::setExtraFeeAmount(double) {
}
