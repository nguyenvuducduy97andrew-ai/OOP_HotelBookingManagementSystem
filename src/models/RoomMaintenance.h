#pragma once

#include <string>

// Represents a half-open maintenance interval: [startDate, endDate).
// The room becomes available again on endDate.
class RoomMaintenance
{
public:
    RoomMaintenance(std::string maintenanceId,
                    std::string roomNumber,
                    std::string startDate,
                    std::string endDate,
                    std::string note = "",
                    std::string status = "Confirmed",
                    std::string createdAt = "");

    const std::string& getMaintenanceId() const;
    const std::string& getRoomNumber() const;
    const std::string& getStartDate() const;
    const std::string& getEndDate() const;
    const std::string& getNote() const;
    const std::string& getStatus() const;
    const std::string& getCreatedAt() const;
    bool isConfirmed() const;
    void setStatus(const std::string& status);

private:
    std::string m_maintenanceId;
    std::string m_roomNumber;
    std::string m_startDate;
    std::string m_endDate;
    std::string m_note;
    std::string m_status;
    std::string m_createdAt;
};
