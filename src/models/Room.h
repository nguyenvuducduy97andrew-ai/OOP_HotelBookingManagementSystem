#pragma once
#include <string>

class Room {
private:
    std::string roomNumber;
    double basePrice;
    bool isAvailable; // Maintenance/inspection status only; does not indicate guest occupancy.

public:
    Room(std::string roomNumber, double basePrice);
    virtual ~Room() {}

    void setRoomNumber(std::string roomNumber);
    std::string getRoomNumber() const;

    void setBasePrice(double basePrice);
    double getBasePrice() const;

    void setIsAvailable(bool isAvailable);
    bool getIsAvailable() const;

    virtual double calculateTargetPrice() = 0;
};