#include "RoomFactory.h"
#include "Room.h"
#include "StandardRoom.h"
#include "SuiteRoom.h"
#include "DeluxeRoom.h"
#include <string>

std::shared_ptr<Room> RoomFactory::createRoom(RoomType type, std::string number, double basePrice) {
    switch (type) {
    case RoomType::Standard:
        return std::make_shared<StandardRoom>(number, basePrice);

    case RoomType::Suite:
        return std::make_shared<SuiteRoom>(number, basePrice, 100.0);

    case RoomType::Deluxe:
        return std::make_shared<DeluxeRoom>(number, basePrice, 50.0);

    default:
        return nullptr;
    }
}