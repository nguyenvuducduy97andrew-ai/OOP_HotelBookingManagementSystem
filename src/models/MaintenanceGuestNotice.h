#pragma once

#include <string>

// Modified: Record an internal maintenance-contact log while delivery remains simulated until an external provider is integrated.
class MaintenanceGuestNotice
{
public:
    MaintenanceGuestNotice(std::string noticeId,
                           std::string maintenanceId,
                           std::string bookingId,
                           std::string channel,
                           std::string status,
                           std::string loggedAt);

    const std::string& getNoticeId() const;
    const std::string& getMaintenanceId() const;
    const std::string& getBookingId() const;
    const std::string& getChannel() const;
    const std::string& getStatus() const;
    const std::string& getLoggedAt() const;
    void setStatus(const std::string& status);

private:
    std::string m_noticeId;
    std::string m_maintenanceId;
    std::string m_bookingId;
    std::string m_channel;
    std::string m_status;
    std::string m_loggedAt;
};
