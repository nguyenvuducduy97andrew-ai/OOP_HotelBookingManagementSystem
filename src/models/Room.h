#pragma once
#include <string>

class Room {
private:
    std::string roomNumber;
    double basePrice;
    bool isAvailable; // Maintenance/inspection status only; does not indicate guest occupancy.
    bool archived = false;

public:
    Room(const std::string& roomNumber, double basePrice);
    virtual ~Room() {}

    void setRoomNumber(const std::string& roomNumber);
    std::string getRoomNumber() const;

    void setBasePrice(double basePrice);
    double getBasePrice() const;

    void setIsAvailable(bool isAvailable);
    bool getIsAvailable() const;

    bool isArchived() const;
    void setArchived(bool archived);

    // Fixed-modified: Expose room type and subtype fee behavior through virtual methods instead of casts.
    virtual double calculateTargetPrice() const = 0;
    virtual std::string getRoomTypeName() const = 0;
    virtual double getExtraFeeAmount() const = 0;
    virtual void setExtraFeeAmount(double fee) = 0;
};