#pragma once

#include <string>
#include "RoomFactory.h"

class HotelManager;

class RoomService
{
public:
    explicit RoomService(HotelManager& hotelManager);
    bool registerRoom(RoomType type, const std::string& roomNumber, double baseRate, std::string& errorMessage);
    bool setAvailability(const std::string& roomNumber, bool available, std::string& errorMessage);
    bool scheduleMaintenance(const std::string& roomNumber, const std::string& startDate,
                             const std::string& endDate, const std::string& note, std::string& errorMessage);
    bool cancelMaintenance(const std::string& maintenanceId, std::string& errorMessage);
    bool confirmMaintenance(const std::string& maintenanceId, std::string& errorMessage);
    bool archiveRoom(const std::string& roomNumber, std::string& errorMessage);
    bool restoreRoom(const std::string& roomNumber, std::string& errorMessage);
    bool deleteRoom(const std::string& roomNumber, std::string& errorMessage);
private:
    HotelManager& m_hotelManager;
};
