#include "ReportService.h"

#include "Booking.h"
#include "Customer.h"
#include "../hotel/HotelManager.h"
#include "Invoice.h"
#include "Room.h"
#include "RoomMaintenance.h"

#include <algorithm>
#include <map>
#include <QTime>
#include <set>

namespace {
bool isInSelectedRange(const QDate& date, const QDate& today, int rangeIndex)
{
    if (!date.isValid()) {
        return false;
    }

    if (rangeIndex == 0) {
        return date == today;
    }
    if (rangeIndex == 1) {
        int dateWeekYear = 0;
        const int dateWeek = date.weekNumber(&dateWeekYear);
        int todayWeekYear = 0;
        const int todayWeek = today.weekNumber(&todayWeekYear);
        return dateWeek == todayWeek && dateWeekYear == todayWeekYear;
    }
    if (rangeIndex == 2) {
        return date.month() == today.month() && date.year() == today.year();
    }
    return date.year() == today.year();
}

QString statusText(BookingState state)
{
    switch (state) {
    case BookingState::UPCOMING: return QStringLiteral("Upcoming");
    case BookingState::ACTIVE: return QStringLiteral("Active");
    case BookingState::COMPLETED: return QStringLiteral("Completed");
    case BookingState::CANCELLED: return QStringLiteral("Cancelled");
    case BookingState::NO_SHOW: return QStringLiteral("Cancelled");
    }
    return QStringLiteral("Unknown");
}

QString formatRangeLabel(const QDate& today, int rangeIndex)
{
    if (rangeIndex == 0) {
        return today.toString("dd MMMM yyyy");
    }
    if (rangeIndex == 1) {
        const QDate weekStart = today.addDays(1 - today.dayOfWeek());
        return QString("%1 — %2").arg(
            weekStart.toString("dd MMM yyyy"),
            weekStart.addDays(6).toString("dd MMM yyyy"));
    }
    if (rangeIndex == 2) {
        return today.toString("MMMM yyyy");
    }
    return QString::number(today.year());
}

struct ReportingWindow
{
    QDateTime start;
    QDateTime endExclusive;
};

ReportingWindow elapsedReportingWindow(const QDateTime& now, int rangeIndex)
{
    const QDate today = now.date();
    if (rangeIndex == 1) {
        return {QDateTime(today.addDays(1 - today.dayOfWeek()), QTime(0, 0)), now};
    }
    if (rangeIndex == 2) {
        return {QDateTime(QDate(today.year(), today.month(), 1), QTime(0, 0)), now};
    }
    if (rangeIndex >= 3) {
        return {QDateTime(QDate(today.year(), 1, 1), QTime(0, 0)), now};
    }
    return {QDateTime(today, QTime(0, 0)), now};
}

// Modified: Planned-operation worklists use the full selected calendar range, while actual KPIs remain limited to elapsed time.
ReportingWindow plannedReportingWindow(const QDateTime& now, int rangeIndex)
{
    const QDate today = now.date();
    if (rangeIndex == 1) {
        const QDate weekStart = today.addDays(1 - today.dayOfWeek());
        return {QDateTime(weekStart, QTime(0, 0)), QDateTime(weekStart.addDays(7), QTime(0, 0))};
    }
    if (rangeIndex == 2) {
        const QDate monthStart(today.year(), today.month(), 1);
        return {QDateTime(monthStart, QTime(0, 0)), QDateTime(monthStart.addMonths(1), QTime(0, 0))};
    }
    if (rangeIndex >= 3) {
        const QDate yearStart(today.year(), 1, 1);
        return {QDateTime(yearStart, QTime(0, 0)), QDateTime(yearStart.addYears(1), QTime(0, 0))};
    }
    return {QDateTime(today, QTime(0, 0)), QDateTime(today.addDays(1), QTime(0, 0))};
}

QDateTime parseTimestamp(const std::string& value, const std::string& legacyDate = {})
{
    QDateTime timestamp = QDateTime::fromString(QString::fromStdString(value), Qt::ISODateWithMs);
    if (!timestamp.isValid()) {
        timestamp = QDateTime::fromString(QString::fromStdString(value), Qt::ISODate);
    }
    if (!timestamp.isValid() && !legacyDate.empty()) {
        timestamp = QDateTime(QDate::fromString(QString::fromStdString(legacyDate), Qt::ISODate), QTime(0, 0));
    }
    return timestamp;
}

qint64 overlapSeconds(const QDateTime& firstStart, const QDateTime& firstEnd,
                      const QDateTime& secondStart, const QDateTime& secondEnd)
{
    const QDateTime start = std::max(firstStart, secondStart);
    const QDateTime end = std::min(firstEnd, secondEnd);
    return start < end ? start.secsTo(end) : 0;
}
}

ReportService::ReportService(const HotelManager* hotelManager)
    : m_hotelManager(hotelManager)
{
}

DashboardReportData ReportService::buildDashboardReport(int rangeIndex, const QString& rangeName) const
{
    DashboardReportData report;
    report.generatedAt = QDateTime::currentDateTime();
    report.rangeName = rangeName;
    const QDate today = report.generatedAt.date();
    report.rangeLabel = formatRangeLabel(today, rangeIndex);
    const ReportingWindow reportingWindow = elapsedReportingWindow(report.generatedAt, rangeIndex);
    const ReportingWindow planningWindow = plannedReportingWindow(report.generatedAt, rangeIndex);

    if (!m_hotelManager) {
        return report;
    }

    std::map<std::string, int> roomBookingCounts;
    std::set<std::string> permanentlySellableRoomNumbers;
    int completedBillableHoursInRange = 0;
    for (const auto& room : m_hotelManager->getRooms()) {
        if (!room || room->isArchived()) {
            continue;
        }

        ++report.totalRooms;
        const std::string typeName = room->getRoomTypeName();
        if (typeName == "Standard") {
            ++report.standardRooms;
        } else if (typeName == "Deluxe") {
            ++report.deluxeRooms;
        } else if (typeName == "Suite") {
            ++report.suiteRooms;
        }
        roomBookingCounts[room->getRoomNumber()] = 0;
        if (room->getIsAvailable()) {
            permanentlySellableRoomNumbers.insert(room->getRoomNumber());
        }
    }

    report.occupiedRooms = static_cast<int>(m_hotelManager->getRoomsByOccupancy(true).size());
    const qint64 reportingSeconds = reportingWindow.start.secsTo(reportingWindow.endExclusive);
    report.periodSaleableRoomHours = static_cast<double>(permanentlySellableRoomNumbers.size() * reportingSeconds) / 3600.0;

    // Modified: Separate Cleaning and Maintenance in reporting while deducting only confirmed physical blocks from saleable capacity.
    for (const RoomMaintenance& block : m_hotelManager->getRoomMaintenances()) {
        const QDateTime blockStart = parseTimestamp(block.getStartAt(), block.getStartDate());
        const QDateTime scheduledBlockEnd = parseTimestamp(block.getEndAt(), block.getEndDate());
        const QDateTime effectiveBlockEnd = parseTimestamp(
            block.getCompletedAt().empty() ? block.getEndAt() : block.getCompletedAt(), block.getEndDate());
        if (!blockStart.isValid() || !scheduledBlockEnd.isValid() || !effectiveBlockEnd.isValid()) {
            continue;
        }

        const qint64 scheduledSeconds = overlapSeconds(
            blockStart, scheduledBlockEnd, planningWindow.start, planningWindow.endExclusive);
        const qint64 elapsedSeconds = overlapSeconds(
            blockStart, effectiveBlockEnd, reportingWindow.start, reportingWindow.endExclusive);

        const auto room = m_hotelManager->findRoomByNumber(block.getRoomNumber());
        const ReportOperationalBlockEntry entry{
            QString::fromStdString(block.getRoomNumber()),
            room ? QString::fromStdString(room->getRoomTypeName()) : QStringLiteral("—"),
            block.isCleaning() ? QStringLiteral("Cleaning") : QStringLiteral("Maintenance"),
            QString::fromStdString(block.getStatus()),
            QString::fromStdString(block.getNote()),
            blockStart,
            // Modified: Planned worklists retain the original block end even when Cleaning was released early; actual hours use the effective end above.
            scheduledBlockEnd
        };
        if (scheduledSeconds > 0) {
            if (block.isCleaning()) {
                report.scheduledCleaning.push_back(entry);
            } else if (block.isMaintenance()) {
                report.maintenanceWindows.push_back(entry);
            }
        }

        if (block.isConfirmed()
            && elapsedSeconds > 0
            && permanentlySellableRoomNumbers.find(block.getRoomNumber()) != permanentlySellableRoomNumbers.end()) {
            const double roomHours = static_cast<double>(elapsedSeconds) / 3600.0;
            if (block.isCleaning()) {
                report.periodCleaningRoomHours += roomHours;
            } else if (block.isMaintenance()) {
                report.periodMaintenanceRoomHours += roomHours;
            }
            report.periodSaleableRoomHours -= roomHours;
        }
    }
    report.periodSaleableRoomHours = std::max(0.0, report.periodSaleableRoomHours);

    for (const auto& booking : m_hotelManager->getBookings()) {
        if (!booking || booking->isDeleted()) {
            continue;
        }

        const BookingState state = m_hotelManager->getBookingState(*booking);
        switch (state) {
        case BookingState::UPCOMING: ++report.upcomingBookings; break;
        case BookingState::ACTIVE: ++report.activeBookings; break;
        case BookingState::COMPLETED: ++report.completedBookings; break;
        case BookingState::CANCELLED: ++report.cancelledBookingsCount; break;
        case BookingState::NO_SHOW: ++report.cancelledBookingsCount; break;
        }

        const QDateTime plannedCheckInAt = parseTimestamp(booking->getPlannedCheckInAt(), booking->getCheckInDate());
        const QDateTime plannedCheckOutAt = parseTimestamp(booking->getPlannedCheckOutAt(), booking->getCheckOutDate());
        const QDateTime actualCheckInAt = parseTimestamp(booking->getActualCheckInAt(), booking->getActualCheckInDate());
        const QDateTime actualCheckOutAt = parseTimestamp(booking->getActualCheckOutAt(), booking->getEffectiveCheckOutDate());
        const QDate actualCheckInDate = actualCheckInAt.date();
        // Modified: Count operational check-ins only after the guest actually arrives, never from a merely scheduled reservation.
        const bool hasActuallyCheckedIn = state == BookingState::ACTIVE || state == BookingState::COMPLETED;
        if (hasActuallyCheckedIn && actualCheckInDate.isValid() && actualCheckInDate.month() == today.month() && actualCheckInDate.year() == today.year()) {
            ++report.bookingsThisMonth;
        }
        if (hasActuallyCheckedIn && actualCheckInDate.isValid() && actualCheckInDate.year() == today.year()) {
            ++report.bookingsThisYear;
        }

        // Modified: Rank rooms by actual arrivals so unarrived reservations and no-shows do not inflate demand reporting.
        if (hasActuallyCheckedIn && isInSelectedRange(actualCheckInDate, today, rangeIndex)) {
            const auto room = booking->getRoom();
            if (room && !room->isArchived()) {
                ++roomBookingCounts[room->getRoomNumber()];
            }
        }

        // Modified: Calculate occupancy in actual room-hours; planned schedules never inflate operational occupancy.
        if (hasActuallyCheckedIn && actualCheckInAt.isValid()) {
            const QDateTime actualEnd = state == BookingState::COMPLETED ? actualCheckOutAt : report.generatedAt;
            if (actualEnd.isValid()) {
                report.periodOccupiedRoomHours += static_cast<double>(overlapSeconds(
                    actualCheckInAt, actualEnd, reportingWindow.start, reportingWindow.endExclusive)) / 3600.0;
            }
        }

        const auto customer = booking->getCustomer();
        const auto room = booking->getRoom();
        const auto invoice = state == BookingState::COMPLETED
            ? m_hotelManager->findInvoiceForBooking(booking->getBookingId())
            : nullptr;
        // Modified: Use immutable invoice data for completed-stay reports when it is available.
        const ReportBookingEntry entry{
            QString::fromStdString(booking->getBookingId()),
            invoice ? QString::fromStdString(invoice->getCustomerNameSnapshot()) : (customer ? QString::fromStdString(customer->getName()) : QStringLiteral("Guest not available")),
            // Modified: Report the guest-facing document number instead of exposing the internal composite customer key.
            invoice ? QString::fromStdString(invoice->getCustomerIdSnapshot()) : (customer ? QString::fromStdString(customer->getDocumentNumber()) : QStringLiteral("—")),
            invoice ? QString::fromStdString(invoice->getCustomerPhoneSnapshot()) : (customer ? QString::fromStdString(customer->getPhoneNumber()) : QStringLiteral("—")),
            invoice ? QString::fromStdString(invoice->getRoomNumberSnapshot()) : (room ? QString::fromStdString(room->getRoomNumber()) : QStringLiteral("—")),
            invoice ? QString::fromStdString(invoice->getRoomTypeSnapshot()) : (room ? QString::fromStdString(room->getRoomTypeName()) : QStringLiteral("—")),
            statusText(state),
            QString::fromStdString(booking->getCancellationReason()),
            plannedCheckInAt,
            plannedCheckOutAt,
            actualCheckInAt,
            actualCheckOutAt
        };

        // Modified: Report planned arrivals and departures from the reservation schedule, independently from actual occupancy and revenue.
        if (state != BookingState::CANCELLED && state != BookingState::NO_SHOW) {
            if (plannedCheckInAt.isValid() && isInSelectedRange(plannedCheckInAt.date(), today, rangeIndex)) {
                report.plannedArrivals.push_back(entry);
            }
            if (plannedCheckOutAt.isValid() && isInSelectedRange(plannedCheckOutAt.date(), today, rangeIndex)) {
                report.plannedDepartures.push_back(entry);
            }
        }

        // Modified: Keep actual arrival and departure worklists separate from their planned counterparts for operational reconciliation.
        if (hasActuallyCheckedIn && actualCheckInAt.isValid()
            && isInSelectedRange(actualCheckInAt.date(), today, rangeIndex)) {
            report.actualCheckIns.push_back(entry);
        }
        if (state == BookingState::COMPLETED && actualCheckOutAt.isValid()
            && isInSelectedRange(actualCheckOutAt.date(), today, rangeIndex)) {
            report.actualCheckOuts.push_back(entry);
        }

        if (state == BookingState::COMPLETED && invoice) {
            const QDate invoiceIssuedDate = QDate::fromString(
                QString::fromStdString(invoice->getInvoiceIssuedDate()), Qt::ISODate);
            // Modified: Place invoiced revenue in the period when the invoice was issued, not when the guest happened to check out.
            if (isInSelectedRange(invoiceIssuedDate, today, rangeIndex)) {
                report.invoicedRevenue += invoice->calculateTotal();
                // Modified: Preserve legacy invoice totals in hourly KPI denominators by mapping each historical nightly unit to 24 report hours.
                completedBillableHoursInRange += invoice->getBillableHours() > 0
                    ? invoice->getBillableHours()
                    : std::max(1, invoice->getNights()) * 24;
            }
        } else if ((state == BookingState::CANCELLED || state == BookingState::NO_SHOW)
                   && isInSelectedRange(plannedCheckInAt.date(), today, rangeIndex)) {
            report.cancelledBookings.push_back(entry);
        }
    }

    if (report.totalRooms > 0) {
        report.occupancyRate = (static_cast<double>(report.occupiedRooms) / report.totalRooms) * 100.0;
    }
    if (report.periodSaleableRoomHours > 0.0) {
        report.periodOccupancyRate = (report.periodOccupiedRoomHours
                                      / report.periodSaleableRoomHours) * 100.0;
    }
    if (completedBillableHoursInRange > 0) {
        report.averageBilledHourlyRate = report.invoicedRevenue / completedBillableHoursInRange;
    }
    if (report.periodSaleableRoomHours > 0.0) {
        report.revenuePerSaleableRoomHour = report.invoicedRevenue / report.periodSaleableRoomHours;
    }

    for (const auto& [roomNumber, bookingCount] : roomBookingCounts) {
        if (bookingCount <= 0) {
            continue;
        }
        // Modified: Do not present zero-demand rooms as "top" rooms when the selected period has no actual arrivals.
        QString roomType = QStringLiteral("Standard");
        if (const auto room = m_hotelManager->findRoomByNumber(roomNumber)) {
            roomType = QString::fromStdString(room->getRoomTypeName());
        }
        report.topRooms.push_back({QString::fromStdString(roomNumber), roomType, bookingCount});
    }

    // Modified and optimized performance: consolidate dashboard report aggregation so widgets consume one stable, sorted reporting snapshot.
    std::sort(report.topRooms.begin(), report.topRooms.end(), [](const ReportRoomEntry& left, const ReportRoomEntry& right) {
        return left.bookingCount != right.bookingCount
            ? left.bookingCount > right.bookingCount
            : left.roomNumber < right.roomNumber;
    });
    const auto byPlannedTime = [](const ReportBookingEntry& left, const ReportBookingEntry& right) {
        return left.plannedCheckInAt != right.plannedCheckInAt
            ? left.plannedCheckInAt < right.plannedCheckInAt
            : left.bookingId < right.bookingId;
    };
    // Modified: Sort planned worklists chronologically so the report can be used directly for shift handover.
    std::sort(report.plannedArrivals.begin(), report.plannedArrivals.end(), byPlannedTime);
    std::sort(report.plannedDepartures.begin(), report.plannedDepartures.end(), [](const ReportBookingEntry& left, const ReportBookingEntry& right) {
        return left.plannedCheckOutAt != right.plannedCheckOutAt
            ? left.plannedCheckOutAt < right.plannedCheckOutAt
            : left.bookingId < right.bookingId;
    });
    std::sort(report.actualCheckIns.begin(), report.actualCheckIns.end(), [](const ReportBookingEntry& left, const ReportBookingEntry& right) {
        return left.actualCheckInAt != right.actualCheckInAt
            ? left.actualCheckInAt < right.actualCheckInAt
            : left.bookingId < right.bookingId;
    });
    std::sort(report.actualCheckOuts.begin(), report.actualCheckOuts.end(), [](const ReportBookingEntry& left, const ReportBookingEntry& right) {
        return left.actualCheckOutAt != right.actualCheckOutAt
            ? left.actualCheckOutAt < right.actualCheckOutAt
            : left.bookingId < right.bookingId;
    });
    const auto byBlockStart = [](const ReportOperationalBlockEntry& left, const ReportOperationalBlockEntry& right) {
        return left.startsAt != right.startsAt ? left.startsAt < right.startsAt : left.roomNumber < right.roomNumber;
    };
    std::sort(report.scheduledCleaning.begin(), report.scheduledCleaning.end(), byBlockStart);
    std::sort(report.maintenanceWindows.begin(), report.maintenanceWindows.end(), byBlockStart);
    std::sort(report.cancelledBookings.begin(), report.cancelledBookings.end(), [](const ReportBookingEntry& left, const ReportBookingEntry& right) {
        return left.plannedCheckInAt != right.plannedCheckInAt ? left.plannedCheckInAt < right.plannedCheckInAt : left.bookingId < right.bookingId;
    });

    return report;
}
