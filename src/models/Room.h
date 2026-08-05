#pragma once
#include <string>

class Room {
private:
    std::string roomNumber;
    double basePrice;
    bool isAvailable; // Maintenance/inspection status only; does not indicate guest occupancy.
    bool archived = false;

    double m_area = 0.0;
    std::string m_bedType = "";
    int m_maxGuests = 0;
    std::string m_description = "";
    std::string m_amenities = "";

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

    double getArea() const;
    void setArea(double area);

    std::string getBedType() const;
    void setBedType(const std::string& bedType);

    int getMaxGuests() const;
    void setMaxGuests(int maxGuests);

    std::string getDescription() const;
    void setDescription(const std::string& description);

    std::string getAmenities() const;
    void setAmenities(const std::string& amenities);

    // Fixed-modified: Expose room type and subtype fee behavior through virtual methods instead of casts.
    virtual double calculateTargetPrice() const = 0;
    virtual std::string getRoomTypeName() const = 0;
    // Modified: Expose sellable occupancy capacity from the room model so reservations cannot exceed a room's limit.
    virtual int getMaximumGuests() const = 0;
    virtual double getExtraFeeAmount() const = 0;
    virtual void setExtraFeeAmount(double fee) = 0;
};
