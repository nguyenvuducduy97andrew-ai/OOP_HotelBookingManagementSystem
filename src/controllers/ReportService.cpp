#include "ReportService.h"

#include "Booking.h"
#include "Customer.h"
#include "HotelManager.h"
#include "Room.h"

#include <algorithm>
#include <map>

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

    if (!m_hotelManager) {
        return report;
    }

    std::map<std::string, int> roomBookingCounts;
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
    }

    report.occupiedRooms = static_cast<int>(m_hotelManager->getRoomsByOccupancy(true).size());

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
        }

        const QDate checkIn = QDate::fromString(QString::fromStdString(booking->getCheckInDate()), Qt::ISODate);
        const QDate checkOut = QDate::fromString(QString::fromStdString(booking->getCheckOutDate()), Qt::ISODate);
        if (checkIn.isValid() && checkIn.month() == today.month() && checkIn.year() == today.year()) {
            ++report.bookingsThisMonth;
        }
        if (checkIn.isValid() && checkIn.year() == today.year()) {
            ++report.bookingsThisYear;
        }

        if (isInSelectedRange(checkIn, today, rangeIndex) && !booking->isCancelled()) {
            const auto room = booking->getRoom();
            if (room && !room->isArchived()) {
                ++roomBookingCounts[room->getRoomNumber()];
            }
        }

        const auto customer = booking->getCustomer();
        const auto room = booking->getRoom();
        const ReportBookingEntry entry{
            QString::fromStdString(booking->getBookingId()),
            customer ? QString::fromStdString(customer->getName()) : QStringLiteral("Guest not available"),
            customer ? QString::fromStdString(customer->getCustomerId()) : QStringLiteral("—"),
            customer ? QString::fromStdString(customer->getPhoneNumber()) : QStringLiteral("—"),
            room ? QString::fromStdString(room->getRoomNumber()) : QStringLiteral("—"),
            room ? QString::fromStdString(room->getRoomTypeName()) : QStringLiteral("—"),
            statusText(state),
            checkIn,
            checkOut
        };

        if (state == BookingState::COMPLETED && isInSelectedRange(checkOut, today, rangeIndex)) {
            report.completedStays.push_back(entry);
        } else if ((state == BookingState::ACTIVE || state == BookingState::UPCOMING)
                   && isInSelectedRange(checkIn, today, rangeIndex)) {
            report.openBookings.push_back(entry);
        } else if (state == BookingState::CANCELLED && isInSelectedRange(checkIn, today, rangeIndex)) {
            report.cancelledBookings.push_back(entry);
        }
    }

    if (report.totalRooms > 0) {
        report.occupancyRate = (static_cast<double>(report.occupiedRooms) / report.totalRooms) * 100.0;
    }

    for (const auto& [roomNumber, bookingCount] : roomBookingCounts) {
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
    std::sort(report.openBookings.begin(), report.openBookings.end(), [](const ReportBookingEntry& left, const ReportBookingEntry& right) {
        return left.checkIn != right.checkIn ? left.checkIn < right.checkIn : left.bookingId < right.bookingId;
    });
    std::sort(report.completedStays.begin(), report.completedStays.end(), [](const ReportBookingEntry& left, const ReportBookingEntry& right) {
        return left.checkOut != right.checkOut ? left.checkOut > right.checkOut : left.bookingId < right.bookingId;
    });
    std::sort(report.cancelledBookings.begin(), report.cancelledBookings.end(), [](const ReportBookingEntry& left, const ReportBookingEntry& right) {
        return left.checkIn != right.checkIn ? left.checkIn < right.checkIn : left.bookingId < right.bookingId;
    });

    return report;
}
