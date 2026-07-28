#include "RoomMaintenance.h"

#include <utility>

RoomMaintenance::RoomMaintenance(std::string maintenanceId,
                                 std::string roomNumber,
                                 std::string startDate,
                                 std::string endDate,
                                 std::string note)
    : m_maintenanceId(std::move(maintenanceId)),
      m_roomNumber(std::move(roomNumber)),
      m_startDate(std::move(startDate)),
      m_endDate(std::move(endDate)),
      m_note(std::move(note))
{
}

const std::string& RoomMaintenance::getMaintenanceId() const { return m_maintenanceId; }
const std::string& RoomMaintenance::getRoomNumber() const { return m_roomNumber; }
const std::string& RoomMaintenance::getStartDate() const { return m_startDate; }
const std::string& RoomMaintenance::getEndDate() const { return m_endDate; }
const std::string& RoomMaintenance::getNote() const { return m_note; }
