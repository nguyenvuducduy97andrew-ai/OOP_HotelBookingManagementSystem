#include "MaintenanceGuestNotice.h"

#include <utility>

MaintenanceGuestNotice::MaintenanceGuestNotice(std::string noticeId,
                                               std::string maintenanceId,
                                               std::string bookingId,
                                               std::string channel,
                                               std::string status,
                                               std::string loggedAt)
    : m_noticeId(std::move(noticeId)),
      m_maintenanceId(std::move(maintenanceId)),
      m_bookingId(std::move(bookingId)),
      m_channel(std::move(channel)),
      m_status(std::move(status)),
      m_loggedAt(std::move(loggedAt))
{
}

const std::string& MaintenanceGuestNotice::getNoticeId() const { return m_noticeId; }
const std::string& MaintenanceGuestNotice::getMaintenanceId() const { return m_maintenanceId; }
const std::string& MaintenanceGuestNotice::getBookingId() const { return m_bookingId; }
const std::string& MaintenanceGuestNotice::getChannel() const { return m_channel; }
const std::string& MaintenanceGuestNotice::getStatus() const { return m_status; }
const std::string& MaintenanceGuestNotice::getLoggedAt() const { return m_loggedAt; }
void MaintenanceGuestNotice::setStatus(const std::string& status) { m_status = status; }
