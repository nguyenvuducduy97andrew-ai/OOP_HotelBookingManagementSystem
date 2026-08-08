#pragma once
#include <string>
#include <memory>
class Room;

enum class RoomType {
    Standard,
    Suite,
    Deluxe
};

class RoomFactory {
public:
    RoomFactory() = delete;
    static std::shared_ptr<Room> createRoom(RoomType type, std::string num, double basePrice);
};
