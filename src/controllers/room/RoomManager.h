#pragma once

#include <memory>
#include <string>
#include <vector>

#include "MaintenanceGuestNotice.h"
#include "Room.h"
#include "RoomFactory.h"
#include "RoomMaintenance.h"

class RoomManager
{
public:
    RoomManager();

    const std::vector<std::shared_ptr<Room>>& getRooms() const;
    std::shared_ptr<Room> findRoomByNumber(const std::string& roomNumber) const;
    bool roomNumberExists(const std::string& roomNumber) const;

    bool registerRoom(RoomType type, const std::string& roomNumber, double baseRate, 
                      double area, const std::string& bedType, int maxGuests, 
                      const std::string& description, const std::string& amenities, 
                      std::string& errorMessage);
    bool updateRoomDetails(const std::string& roomNumber, double baseRate, double extraFee,
                           double area, const std::string& bedType, int maxGuests,
                           const std::string& description, const std::string& amenities,
                           std::string& errorMessage);
    bool setRoomAvailability(const std::string& roomNumber, bool available, std::string& errorMessage);
    bool scheduleRoomMaintenance(const std::string& roomNumber,
                                 const std::string& startDate,
                                 const std::string& endDate,
                                 const std::string& note,
                                 const std::vector<std::string>& affectedBookingIds,
                                 std::string& errorMessage);
    bool cancelRoomMaintenance(const std::string& maintenanceId, std::string& errorMessage);
    bool confirmRoomMaintenance(const std::string& maintenanceId, std::string& errorMessage);
    bool archiveRoom(const std::string& roomNumber, std::string& errorMessage);
    bool restoreRoom(const std::string& roomNumber, std::string& errorMessage);
    bool deleteRoom(const std::string& roomNumber, std::string& errorMessage);

    const std::vector<RoomMaintenance>& getRoomMaintenances() const;
    const std::vector<MaintenanceGuestNotice>& getMaintenanceGuestNotices() const;
    std::vector<MaintenanceGuestNotice> getMaintenanceGuestNotices(const std::string& maintenanceId) const;
    bool isRoomUnderMaintenance(const std::string& roomNumber, const std::string& date) const;
    bool isRoomBlockedAt(const std::string& roomNumber, const std::string& at) const;
    bool startCleaningAfterCheckout(const std::string& roomNumber,
                                   const std::string& actualCheckoutAt,
                                   std::string& errorMessage);
    bool markRoomReady(const std::string& roomNumber,
                       const std::string& readyAt,
                       const std::string& completedBy,
                       std::string& errorMessage);
    bool hasRoomMaintenanceConflict(const std::string& roomNumber,
                                    const std::string& startDate,
                                    const std::string& endDate,
                                    std::string& errorMessage) const;

    bool restoreRoomMaintenanceFromDatabase(const std::string& maintenanceId,
                                            const std::string& roomNumber,
                                            const std::string& startDate,
                                            const std::string& endDate,
                                            const std::string& note,
                                            const std::string& status,
                                            const std::string& createdAt,
                                            const std::string& blockType,
                                            const std::string& startAt,
                                            const std::string& endAt,
                                            const std::string& completedAt,
                                            const std::string& completedBy,
                                            std::string& errorMessage);
    bool validateRoomMaintenanceRestoration(const std::string& maintenanceId,
                                            const std::string& roomNumber,
                                            const std::string& startDate,
                                            const std::string& endDate,
                                            const std::string& status,
                                            const std::string& blockType,
                                            const std::string& startAt,
                                            const std::string& endAt,
                                            std::string& errorMessage) const;

    bool restoreMaintenanceGuestNoticeFromDatabase(const std::string& noticeId,
                                                   const std::string& maintenanceId,
                                                   const std::string& bookingId,
                                                   const std::string& channel,
                                                   const std::string& status,
                                                   const std::string& loggedAt,
                                                   std::string& errorMessage);

    void clearAll();

private:
    bool isValidRoomNumber(const std::string& roomNumber) const;
    bool validateRoomInput(const std::string& roomNumber,
                           double baseRate,
                           std::string& errorMessage) const;
    RoomMaintenance* findMaintenanceById(const std::string& maintenanceId);
    const RoomMaintenance* findMaintenanceById(const std::string& maintenanceId) const;

    std::vector<std::shared_ptr<Room>> m_rooms;
    std::vector<RoomMaintenance> m_roomMaintenances;
    std::vector<MaintenanceGuestNotice> m_maintenanceGuestNotices;
};
