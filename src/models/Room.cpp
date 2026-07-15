#include "Room.h"
#include <string>

Room::Room(std::string roomNumber, double basePrice)
    : roomNumber(roomNumber),
      basePrice(basePrice),
      isAvailable(true),
      archived(false) {
}
void Room::setRoomNumber(std::string roomNumber) {
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