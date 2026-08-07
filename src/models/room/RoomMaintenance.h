#pragma once

#include <string>

// Represents an operational room block. The persisted time interval is half-open: [startAt, endAt).
// Maintenance and Cleaning intentionally share one model because both reserve the same room resource,
// while blockType preserves their different operational and reporting meanings.
class RoomMaintenance
{
public:
    RoomMaintenance(std::string maintenanceId,
                    std::string roomNumber,
                    std::string startDate,
                    std::string endDate,
                    std::string note = "",
                    std::string status = "Confirmed",
                    std::string createdAt = "",
                    std::string blockType = "Maintenance",
                    std::string startAt = "",
                    std::string endAt = "",
                    std::string completedAt = "",
                    std::string completedBy = "");

    const std::string& getMaintenanceId() const;
    const std::string& getRoomNumber() const;
    const std::string& getStartDate() const;
    const std::string& getEndDate() const;
    const std::string& getNote() const;
    const std::string& getStatus() const;
    const std::string& getCreatedAt() const;
    const std::string& getBlockType() const;
    const std::string& getStartAt() const;
    const std::string& getEndAt() const;
    const std::string& getCompletedAt() const;
    const std::string& getCompletedBy() const;
    bool isConfirmed() const;
    bool isMaintenance() const;
    bool isCleaning() const;
    void setStatus(const std::string& status);
    void setCompletedAt(const std::string& completedAt);
    void setCompletedBy(const std::string& completedBy);

private:
    std::string m_maintenanceId;
    std::string m_roomNumber;
    std::string m_startDate;
    std::string m_endDate;
    std::string m_note;
    std::string m_status;
    std::string m_createdAt;
    std::string m_blockType;
    std::string m_startAt;
    std::string m_endAt;
    std::string m_completedAt;
    std::string m_completedBy;
};
