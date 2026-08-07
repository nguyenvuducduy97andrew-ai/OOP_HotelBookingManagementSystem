#include "RoomMaintenance.h"

#include <utility>

RoomMaintenance::RoomMaintenance(std::string maintenanceId,
                                 std::string roomNumber,
                                 std::string startDate,
                                 std::string endDate,
                                 std::string note,
                                 std::string status,
                                 std::string createdAt,
                                 std::string blockType,
                                 std::string startAt,
                                 std::string endAt,
                                 std::string completedAt,
                                 std::string completedBy)
    : m_maintenanceId(std::move(maintenanceId)),
      m_roomNumber(std::move(roomNumber)),
      m_startDate(std::move(startDate)),
      m_endDate(std::move(endDate)),
      m_note(std::move(note)),
      m_status(std::move(status)),
      m_createdAt(std::move(createdAt)),
      m_blockType(std::move(blockType)),
      m_startAt(std::move(startAt)),
      m_endAt(std::move(endAt)),
      m_completedAt(std::move(completedAt)),
      m_completedBy(std::move(completedBy))
{
}

const std::string& RoomMaintenance::getMaintenanceId() const { return m_maintenanceId; }
const std::string& RoomMaintenance::getRoomNumber() const { return m_roomNumber; }
const std::string& RoomMaintenance::getStartDate() const { return m_startDate; }
const std::string& RoomMaintenance::getEndDate() const { return m_endDate; }
const std::string& RoomMaintenance::getNote() const { return m_note; }
const std::string& RoomMaintenance::getStatus() const { return m_status; }
const std::string& RoomMaintenance::getCreatedAt() const { return m_createdAt; }
const std::string& RoomMaintenance::getBlockType() const { return m_blockType; }
const std::string& RoomMaintenance::getStartAt() const { return m_startAt; }
const std::string& RoomMaintenance::getEndAt() const { return m_endAt; }
const std::string& RoomMaintenance::getCompletedAt() const { return m_completedAt; }
const std::string& RoomMaintenance::getCompletedBy() const { return m_completedBy; }
bool RoomMaintenance::isConfirmed() const { return m_status == "Confirmed"; }
bool RoomMaintenance::isMaintenance() const { return m_blockType == "Maintenance"; }
bool RoomMaintenance::isCleaning() const { return m_blockType == "Cleaning"; }
void RoomMaintenance::setStatus(const std::string& status) { m_status = status; }
void RoomMaintenance::setCompletedAt(const std::string& completedAt) { m_completedAt = completedAt; }
void RoomMaintenance::setCompletedBy(const std::string& completedBy) { m_completedBy = completedBy; }
