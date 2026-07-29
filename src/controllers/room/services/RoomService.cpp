#include "RoomService.h"
#include "../../hotel/HotelManager.h"

RoomService::RoomService(HotelManager& hotelManager) : m_hotelManager(hotelManager) {}
bool RoomService::registerRoom(RoomType type, const std::string& roomNumber, double baseRate, std::string& errorMessage) { return m_hotelManager.registerRoomCore(type, roomNumber, baseRate, errorMessage); }
bool RoomService::setAvailability(const std::string& roomNumber, bool available, std::string& errorMessage) { return m_hotelManager.setRoomAvailabilityCore(roomNumber, available, errorMessage); }
bool RoomService::scheduleMaintenance(const std::string& roomNumber, const std::string& startDate, const std::string& endDate, const std::string& note, std::string& errorMessage) { return m_hotelManager.scheduleRoomMaintenanceCore(roomNumber, startDate, endDate, note, errorMessage); }
bool RoomService::cancelMaintenance(const std::string& maintenanceId, std::string& errorMessage) { return m_hotelManager.cancelRoomMaintenanceCore(maintenanceId, errorMessage); }
bool RoomService::confirmMaintenance(const std::string& maintenanceId, std::string& errorMessage) { return m_hotelManager.confirmRoomMaintenanceCore(maintenanceId, errorMessage); }
bool RoomService::archiveRoom(const std::string& roomNumber, std::string& errorMessage) { return m_hotelManager.archiveRoomCore(roomNumber, errorMessage); }
bool RoomService::restoreRoom(const std::string& roomNumber, std::string& errorMessage) { return m_hotelManager.restoreRoomCore(roomNumber, errorMessage); }
bool RoomService::deleteRoom(const std::string& roomNumber, std::string& errorMessage) { return m_hotelManager.deleteRoomCore(roomNumber, errorMessage); }
