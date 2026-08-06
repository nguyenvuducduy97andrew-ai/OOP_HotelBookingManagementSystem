#include "RoomManager.h"

#include <QDate>
#include <QDateTime>
#include <QTime>
#include <QUuid>
#include <QRegularExpression>
#include <QString>

#include <algorithm>
#include <cctype>
#include <utility>

namespace {
bool isValidRoomNumber(const std::string& roomNumber)
{
    return !roomNumber.empty() &&
           std::all_of(roomNumber.begin(), roomNumber.end(), [](unsigned char c) {
               return std::isalnum(c);
           });
}

QDateTime parseIsoDateTime(const std::string& value)
{
    QDateTime parsed = QDateTime::fromString(QString::fromStdString(value), Qt::ISODateWithMs);
    if (!parsed.isValid()) {
        parsed = QDateTime::fromString(QString::fromStdString(value), Qt::ISODate);
    }
    return parsed;
}

QDateTime startOfLegacyDate(const std::string& date)
{
    const QDate parsed = QDate::fromString(QString::fromStdString(date), Qt::ISODate);
    return parsed.isValid() ? QDateTime(parsed, QTime(0, 0)) : QDateTime();
}

QDateTime blockStart(const RoomMaintenance& block)
{
    const QDateTime timedStart = parseIsoDateTime(block.getStartAt());
    return timedStart.isValid() ? timedStart : startOfLegacyDate(block.getStartDate());
}

QDateTime blockEffectiveEnd(const RoomMaintenance& block)
{
    QDateTime end = parseIsoDateTime(block.getEndAt());
    if (!end.isValid()) {
        end = startOfLegacyDate(block.getEndDate());
    }
    const QDateTime completed = parseIsoDateTime(block.getCompletedAt());
    if (completed.isValid() && (!end.isValid() || completed < end)) {
        return completed;
    }
    return end;
}
}

RoomManager::RoomManager() = default;

const std::vector<std::shared_ptr<Room>>& RoomManager::getRooms() const
{
    return m_rooms;
}

std::shared_ptr<Room> RoomManager::findRoomByNumber(const std::string& roomNumber) const
{
    for (const auto& room : m_rooms) {
        if (room && room->getRoomNumber() == roomNumber) {
            return room;
        }
    }
    return nullptr;
}

bool RoomManager::roomNumberExists(const std::string& roomNumber) const
{
    return findRoomByNumber(roomNumber) != nullptr;
}

bool RoomManager::isValidRoomNumber(const std::string& roomNumber) const
{
    return !roomNumber.empty() &&
           std::all_of(roomNumber.begin(), roomNumber.end(), [](unsigned char c) {
               return std::isalnum(c);
           });
}

bool RoomManager::validateRoomInput(const std::string& roomNumber, double baseRate, std::string& errorMessage) const
{
    if (!isValidRoomNumber(roomNumber)) {
        errorMessage = "Room number must not be empty and must contain only letters and numbers.";
        return false;
    }

    if (baseRate <= 0) {
        errorMessage = "Base rate must be greater than zero.";
        return false;
    }

    if (roomNumberExists(roomNumber)) {
        errorMessage = "Room number already exists.";
        return false;
    }

    return true;
}

bool RoomManager::registerRoom(RoomType type, const std::string& roomNumber, double baseRate, 
                               double area, const std::string& bedType, int maxGuests, 
                               const std::string& description, const std::string& amenities, 
                               std::string& errorMessage)
{
    if (!validateRoomInput(roomNumber, baseRate, errorMessage)) {
        return false;
    }

    auto room = RoomFactory::createRoom(type, roomNumber, baseRate);
    if (!room) {
        errorMessage = "Failed to create room.";
        return false;
    }

    room->setArea(area);
    room->setBedType(bedType);
    room->setMaxGuests(maxGuests);
    room->setDescription(description);
    room->setAmenities(amenities);

    m_rooms.push_back(room);
    return true;
}

bool RoomManager::updateRoomDetails(const std::string& roomNumber, double baseRate, double extraFee,
                                    double area, const std::string& bedType, int maxGuests,
                                    const std::string& description, const std::string& amenities,
                                    std::string& errorMessage)
{
    const auto room = findRoomByNumber(roomNumber);
    if (!room) {
        errorMessage = "Room not found.";
        return false;
    }

    room->setBasePrice(baseRate);
    room->setExtraFeeAmount(extraFee);
    room->setArea(area);
    room->setBedType(bedType);
    room->setMaxGuests(maxGuests);
    room->setDescription(description);
    room->setAmenities(amenities);
    return true;
}

bool RoomManager::setRoomAvailability(const std::string& roomNumber, bool available, std::string& errorMessage)
{
    const auto room = findRoomByNumber(roomNumber);
    if (!room) {
        errorMessage = "Room not found.";
        return false;
    }

    room->setIsAvailable(available);
    return true;
}

bool RoomManager::hasRoomMaintenanceConflict(const std::string& roomNumber,
                                             const std::string& startDate,
                                             const std::string& endDate,
                                             std::string& errorMessage) const
{
    const QDate legacyStart = QDate::fromString(QString::fromStdString(startDate), Qt::ISODate);
    const QDate legacyEnd = QDate::fromString(QString::fromStdString(endDate), Qt::ISODate);
    if (!legacyStart.isValid() || !legacyEnd.isValid() || legacyEnd <= legacyStart) {
        errorMessage = "Maintenance availability requires a valid ISO date range.";
        return true;
    }

    for (const RoomMaintenance& maintenance : m_roomMaintenances) {
        if (!maintenance.isMaintenance() || maintenance.getRoomNumber() != roomNumber) {
            continue;
        }

        if (startDate < maintenance.getEndDate() && maintenance.getStartDate() < endDate) {
            errorMessage = "Room " + roomNumber + " has a " + maintenance.getStatus() + " maintenance case from "
                + maintenance.getStartDate() + " to " + maintenance.getEndDate() + ".";
            return true;
        }
    }

    return false;
}

bool RoomManager::isRoomUnderMaintenance(const std::string& roomNumber, const std::string& date) const
{
    return std::any_of(m_roomMaintenances.cbegin(), m_roomMaintenances.cend(),
        [&roomNumber, &date](const RoomMaintenance& maintenance) {
            return maintenance.isConfirmed() && maintenance.getRoomNumber() == roomNumber
                && maintenance.getStartDate() <= date
                && date < maintenance.getEndDate();
        });
}

bool RoomManager::isRoomBlockedAt(const std::string& roomNumber, const std::string& at) const
{
    const QDateTime requestedAt = parseIsoDateTime(at);
    if (!requestedAt.isValid()) {
        return false;
    }

    return std::any_of(m_roomMaintenances.cbegin(), m_roomMaintenances.cend(),
        [&roomNumber, &requestedAt](const RoomMaintenance& block) {
            const QDateTime start = blockStart(block);
            const QDateTime end = blockEffectiveEnd(block);
            return block.isConfirmed() && block.getRoomNumber() == roomNumber
                && start.isValid() && end.isValid() && start <= requestedAt && requestedAt < end;
        });
}

bool RoomManager::startCleaningAfterCheckout(const std::string& roomNumber,
                                             const std::string& actualCheckoutAt,
                                             std::string& errorMessage)
{
    const auto room = findRoomByNumber(roomNumber);
    if (!room || room->isArchived()) {
        errorMessage = "Cannot start cleaning for an unavailable room.";
        return false;
    }

    const QDateTime checkoutAt = parseIsoDateTime(actualCheckoutAt);
    if (!checkoutAt.isValid()) {
        errorMessage = "Cleaning requires a valid actual checkout timestamp.";
        return false;
    }

    const QDateTime cleaningEnd = checkoutAt.addSecs(2 * 60 * 60);
    const std::string startDate = checkoutAt.date().toString(Qt::ISODate).toStdString();
    const std::string endDate = cleaningEnd.date().addDays(1).toString(Qt::ISODate).toStdString();
    const std::string timestamp = QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toStdString();

    // Modified: Create a distinct Cleaning block after checkout so the interval engine reserves the two-hour turnover window.
    m_roomMaintenances.emplace_back(
        "CLN-" + QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString(),
        roomNumber, startDate, endDate, "Automatic post-checkout cleaning", "Confirmed", timestamp,
        "Cleaning", checkoutAt.toString(Qt::ISODateWithMs).toStdString(),
        cleaningEnd.toString(Qt::ISODateWithMs).toStdString());
    return true;
}

bool RoomManager::markRoomReady(const std::string& roomNumber,
                                const std::string& readyAt,
                                const std::string& completedBy,
                                std::string& errorMessage)
{
    const QDateTime completion = parseIsoDateTime(readyAt);
    if (!completion.isValid()) {
        errorMessage = "Room-ready time must be a valid timestamp.";
        return false;
    }

    for (auto& block : m_roomMaintenances) {
        if (!block.isCleaning() || !block.isConfirmed() || block.getRoomNumber() != roomNumber) {
            continue;
        }
        const QDateTime start = blockStart(block);
        const QDateTime end = blockEffectiveEnd(block);
        if (start.isValid() && end.isValid() && start <= completion && completion < end) {
            // Modified: Finishing Cleaning early releases the room without rewriting its planned two-hour operational record.
            block.setCompletedAt(completion.toString(Qt::ISODateWithMs).toStdString());
            block.setCompletedBy(completedBy.empty() ? "Staff" : completedBy);
            return true;
        }
    }

    errorMessage = "No active cleaning block was found for this room.";
    return false;
}

RoomMaintenance* RoomManager::findMaintenanceById(const std::string& maintenanceId)
{
    for (auto& maintenance : m_roomMaintenances) {
        if (maintenance.getMaintenanceId() == maintenanceId) {
            return &maintenance;
        }
    }
    return nullptr;
}

const RoomMaintenance* RoomManager::findMaintenanceById(const std::string& maintenanceId) const
{
    for (const auto& maintenance : m_roomMaintenances) {
        if (maintenance.getMaintenanceId() == maintenanceId) {
            return &maintenance;
        }
    }
    return nullptr;
}

bool RoomManager::scheduleRoomMaintenance(const std::string& roomNumber,
                                          const std::string& startDate,
                                          const std::string& endDate,
                                          const std::string& note,
                                          const std::vector<std::string>& affectedBookingIds,
                                          std::string& errorMessage)
{
    const auto room = findRoomByNumber(roomNumber);
    if (!room) {
        errorMessage = "Room not found.";
        return false;
    }
    if (room->isArchived()) {
        errorMessage = "Cannot schedule maintenance for an archived room.";
        return false;
    }

    const QDate start = QDate::fromString(QString::fromStdString(startDate), Qt::ISODate);
    const QDate end = QDate::fromString(QString::fromStdString(endDate), Qt::ISODate);
    if (!start.isValid() || !end.isValid() || start < QDate::currentDate() || end <= start) {
        errorMessage = "Maintenance must start today or later and end after its start date.";
        return false;
    }

    for (const RoomMaintenance& maintenance : m_roomMaintenances) {
        if (maintenance.isMaintenance() && maintenance.getRoomNumber() == roomNumber
            && startDate < maintenance.getEndDate() && maintenance.getStartDate() < endDate) {
            errorMessage = "Room " + roomNumber + " already has a maintenance case from "
                + maintenance.getStartDate() + " to " + maintenance.getEndDate() + ".";
            return false;
        }
    }

    const std::string maintenanceId = "MTN-" + QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    const std::string timestamp = QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toStdString();
    const std::string status = affectedBookingIds.empty() ? "Confirmed" : "Awaiting guest response";
    // Modified: Preserve full-day Maintenance as an operational block, with timestamps available to the shared interval engine.
    m_roomMaintenances.emplace_back(maintenanceId, roomNumber, startDate, endDate, note, status, timestamp,
                                    "Maintenance", startDate + "T00:00:00", endDate + "T00:00:00");

    for (const std::string& bookingId : affectedBookingIds) {
        m_maintenanceGuestNotices.emplace_back(
            "NTC-" + QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString(),
            maintenanceId,
            bookingId,
            "Simulated email",
            "Awaiting guest response",
            timestamp);
    }

    if (!affectedBookingIds.empty()) {
        const std::string bookingLabel = affectedBookingIds.size() == 1 ? "booking" : "bookings";
        errorMessage = "Maintenance case created. Simulated guest notifications were logged for "
            + std::to_string(affectedBookingIds.size()) + " affected " + bookingLabel
            + ". Resolve them, then confirm the case.";
    }

    return true;
}

bool RoomManager::cancelRoomMaintenance(const std::string& maintenanceId, std::string& errorMessage)
{
    const auto matchingMaintenance = std::find_if(m_roomMaintenances.cbegin(), m_roomMaintenances.cend(),
        [&maintenanceId](const RoomMaintenance& maintenance) {
            return maintenance.getMaintenanceId() == maintenanceId;
        });
    if (matchingMaintenance == m_roomMaintenances.cend()) {
        errorMessage = "Maintenance schedule not found.";
        return false;
    }

    m_maintenanceGuestNotices.erase(std::remove_if(m_maintenanceGuestNotices.begin(), m_maintenanceGuestNotices.end(),
        [&maintenanceId](const MaintenanceGuestNotice& notice) {
            return notice.getMaintenanceId() == maintenanceId;
        }), m_maintenanceGuestNotices.end());
    m_roomMaintenances.erase(matchingMaintenance);
    return true;
}

bool RoomManager::confirmRoomMaintenance(const std::string& maintenanceId, std::string& errorMessage)
{
    auto matchingMaintenance = findMaintenanceById(maintenanceId);
    if (!matchingMaintenance) {
        errorMessage = "Maintenance case not found.";
        return false;
    }
    if (matchingMaintenance->isConfirmed()) {
        errorMessage = "Maintenance is already confirmed.";
        return false;
    }

    matchingMaintenance->setStatus("Confirmed");
    for (auto& notice : m_maintenanceGuestNotices) {
        if (notice.getMaintenanceId() == maintenanceId) {
            notice.setStatus("Resolved — booking impact handled");
        }
    }
    return true;
}

bool RoomManager::archiveRoom(const std::string& roomNumber, std::string& errorMessage)
{
    auto room = findRoomByNumber(roomNumber);
    if (!room) {
        errorMessage = "Room not found.";
        return false;
    }

    room->setArchived(true);
    return true;
}

bool RoomManager::restoreRoom(const std::string& roomNumber, std::string& errorMessage)
{
    auto room = findRoomByNumber(roomNumber);
    if (!room) {
        errorMessage = "Room not found.";
        return false;
    }

    room->setArchived(false);
    return true;
}

bool RoomManager::deleteRoom(const std::string& roomNumber, std::string& errorMessage)
{
    auto room = findRoomByNumber(roomNumber);
    if (!room) {
        errorMessage = "Room not found.";
        return false;
    }

    m_rooms.erase(std::remove(m_rooms.begin(), m_rooms.end(), room), m_rooms.end());
    return true;
}

const std::vector<RoomMaintenance>& RoomManager::getRoomMaintenances() const
{
    return m_roomMaintenances;
}

const std::vector<MaintenanceGuestNotice>& RoomManager::getMaintenanceGuestNotices() const
{
    return m_maintenanceGuestNotices;
}

std::vector<MaintenanceGuestNotice> RoomManager::getMaintenanceGuestNotices(const std::string& maintenanceId) const
{
    std::vector<MaintenanceGuestNotice> matches;
    for (const auto& notice : m_maintenanceGuestNotices) {
        if (notice.getMaintenanceId() == maintenanceId) {
            matches.push_back(notice);
        }
    }
    return matches;
}

bool RoomManager::restoreRoomMaintenanceFromDatabase(const std::string& maintenanceId,
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
                                                     std::string& errorMessage)
{
    if (!validateRoomMaintenanceRestoration(
            maintenanceId, roomNumber, startDate, endDate, status, blockType, startAt, endAt, errorMessage)) {
        return false;
    }

    m_roomMaintenances.emplace_back(maintenanceId, roomNumber, startDate, endDate, note, status, createdAt,
                                    blockType, startAt, endAt, completedAt, completedBy);
    return true;
}

bool RoomManager::validateRoomMaintenanceRestoration(const std::string& maintenanceId,
                                                     const std::string& roomNumber,
                                                     const std::string& startDate,
                                                     const std::string& endDate,
                                                     const std::string& status,
                                                     const std::string& blockType,
                                                     const std::string& startAt,
                                                     const std::string& endAt,
                                                     std::string& errorMessage) const
{
    if (maintenanceId.empty()) {
        errorMessage = "Persisted maintenance ID is empty.";
        return false;
    }
    if (!findRoomByNumber(roomNumber)) {
        errorMessage = "Persisted maintenance references a missing room.";
        return false;
    }

    const QDate persistedStartDate = QDate::fromString(QString::fromStdString(startDate), Qt::ISODate);
    const QDate persistedEndDate = QDate::fromString(QString::fromStdString(endDate), Qt::ISODate);
    if (!persistedStartDate.isValid() || !persistedEndDate.isValid() || persistedEndDate <= persistedStartDate) {
        errorMessage = "Persisted maintenance dates are invalid.";
        return false;
    }

    if (std::any_of(m_roomMaintenances.cbegin(), m_roomMaintenances.cend(),
                    [&maintenanceId](const RoomMaintenance& maintenance) {
                        return maintenance.getMaintenanceId() == maintenanceId;
                    })) {
        errorMessage = "Duplicate persisted maintenance ID: " + maintenanceId;
        return false;
    }

    if (status != "Confirmed" && status != "Awaiting guest response") {
        errorMessage = "Persisted maintenance has an invalid status.";
        return false;
    }
    if (blockType != "Maintenance" && blockType != "Cleaning") {
        errorMessage = "Persisted room block has an invalid type.";
        return false;
    }
    const QDateTime start = parseIsoDateTime(startAt);
    const QDateTime end = parseIsoDateTime(endAt);
    if (!start.isValid() || !end.isValid() || end <= start) {
        errorMessage = "Persisted room block timestamps are invalid.";
        return false;
    }

    return true;
}

bool RoomManager::restoreMaintenanceGuestNoticeFromDatabase(const std::string& noticeId,
                                                            const std::string& maintenanceId,
                                                            const std::string& bookingId,
                                                            const std::string& channel,
                                                            const std::string& status,
                                                            const std::string& loggedAt,
                                                            std::string& errorMessage)
{
    if (noticeId.empty() || maintenanceId.empty() || bookingId.empty() || channel.empty() || status.empty()) {
        errorMessage = "Persisted maintenance notification is incomplete.";
        return false;
    }

    const bool maintenanceExists = std::any_of(m_roomMaintenances.cbegin(), m_roomMaintenances.cend(),
        [&maintenanceId](const RoomMaintenance& maintenance) {
            return maintenance.getMaintenanceId() == maintenanceId;
        });
    if (!maintenanceExists) {
        errorMessage = "Persisted maintenance notification has a missing maintenance case or booking.";
        return false;
    }

    if (std::any_of(m_maintenanceGuestNotices.cbegin(), m_maintenanceGuestNotices.cend(),
                    [&noticeId](const MaintenanceGuestNotice& notice) {
                        return notice.getNoticeId() == noticeId;
                    })) {
        errorMessage = "Duplicate persisted maintenance notification ID: " + noticeId;
        return false;
    }

    m_maintenanceGuestNotices.emplace_back(noticeId, maintenanceId, bookingId, channel, status, loggedAt);
    return true;
}

void RoomManager::clearAll()
{
    m_rooms.clear();
    m_roomMaintenances.clear();
    m_maintenanceGuestNotices.clear();
}
