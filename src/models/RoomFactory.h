#pragma once
#include <string>
#include <iostream>
#include <memory>
class Room;

enum class RoomType {
    Standard,
    Suite,
    Deluxe
};

class RoomFactory {
public:
    std::shared_ptr<Room> createRoom(RoomType type, std::string num, double basePrice);
};