#include "RoomManager.h"
#include "../hotel/HotelManager.h"

RoomManager::RoomManager(HotelManager& hotelManager) : m_roomService(hotelManager) {}
bool RoomManager::registerRoom(RoomType type, const std::string& roomNumber, double baseRate, std::string& errorMessage) { return m_roomService.registerRoom(type, roomNumber, baseRate, errorMessage); }
bool RoomManager::setAvailability(const std::string& roomNumber, bool available, std::string& errorMessage) { return m_roomService.setAvailability(roomNumber, available, errorMessage); }
bool RoomManager::scheduleMaintenance(const std::string& roomNumber, const std::string& startDate, const std::string& endDate, const std::string& note, std::string& errorMessage) { return m_roomService.scheduleMaintenance(roomNumber, startDate, endDate, note, errorMessage); }
bool RoomManager::cancelMaintenance(const std::string& maintenanceId, std::string& errorMessage) { return m_roomService.cancelMaintenance(maintenanceId, errorMessage); }
bool RoomManager::archiveRoom(const std::string& roomNumber, std::string& errorMessage) { return m_roomService.archiveRoom(roomNumber, errorMessage); }
bool RoomManager::restoreRoom(const std::string& roomNumber, std::string& errorMessage) { return m_roomService.restoreRoom(roomNumber, errorMessage); }
bool RoomManager::deleteRoom(const std::string& roomNumber, std::string& errorMessage) { return m_roomService.deleteRoom(roomNumber, errorMessage); }
