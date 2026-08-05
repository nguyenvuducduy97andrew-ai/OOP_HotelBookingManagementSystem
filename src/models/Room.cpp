#include "Room.h"
#include <string>

Room::Room(const std::string& roomNumber, double basePrice)
    : roomNumber(roomNumber),
      basePrice(basePrice),
      isAvailable(true),
      archived(false) {
}
void Room::setRoomNumber(const std::string& roomNumber) {
    this->roomNumber = roomNumber;
}

std::string Room::getRoomNumber() const {
    return this->roomNumber;
}

void Room::setBasePrice(double basePrice) {
    this->basePrice = basePrice;
}

double Room::getBasePrice() const {
    return this->basePrice;
}

void Room::setIsAvailable(bool isAvailable) {
    this->isAvailable = isAvailable;
}

bool Room::getIsAvailable() const {
    return this->isAvailable;
}

bool Room::isArchived() const {
    return archived;
}

void Room::setArchived(bool archived) {
    this->archived = archived;
}

double Room::getArea() const { return m_area; }
void Room::setArea(double area) { m_area = area; }

std::string Room::getBedType() const { return m_bedType; }
void Room::setBedType(const std::string& bedType) { m_bedType = bedType; }

int Room::getMaxGuests() const { return m_maxGuests; }
void Room::setMaxGuests(int maxGuests) { m_maxGuests = maxGuests; }

std::string Room::getDescription() const { return m_description; }
void Room::setDescription(const std::string& description) { m_description = description; }

std::string Room::getAmenities() const { return m_amenities; }
void Room::setAmenities(const std::string& amenities) { m_amenities = amenities; }