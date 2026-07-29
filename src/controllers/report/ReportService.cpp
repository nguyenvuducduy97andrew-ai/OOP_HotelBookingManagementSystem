#include "ReportService.h"

#include "Booking.h"
#include "Customer.h"
#include "../hotel/HotelManager.h"
#include "Invoice.h"
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
    case BookingState::NO_SHOW: return QStringLiteral("No-show");
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
    QDate start;
    QDate endExclusive;
};

ReportingWindow elapsedReportingWindow(const QDate& today, int rangeIndex)
{
    if (rangeIndex == 1) {
        return {today.addDays(1 - today.dayOfWeek()), today.addDays(1)};
    }
    if (rangeIndex == 2) {
        return {QDate(today.year(), today.month(), 1), today.addDays(1)};
    }
    if (rangeIndex >= 3) {
        return {QDate(today.year(), 1, 1), today.addDays(1)};
    }
    return {today, today.addDays(1)};
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
    const ReportingWindow reportingWindow = elapsedReportingWindow(today, rangeIndex);
    report.periodDaysToDate = reportingWindow.start.daysTo(reportingWindow.endExclusive);

    if (!m_hotelManager) {
        return report;
    }

    std::map<std::string, int> roomBookingCounts;
    int completedNightsInRange = 0;
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
    report.periodAvailableRoomNights = report.totalRooms * report.periodDaysToDate;

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
        case BookingState::NO_SHOW: ++report.noShowBookingsCount; break;
        }

        const QDate checkIn = QDate::fromString(QString::fromStdString(
            booking->getActualCheckInDate().empty() ? booking->getCheckInDate() : booking->getActualCheckInDate()), Qt::ISODate);
        // Modified: Completed-stay reports use the recorded departure rather than the reservation's planned departure.
        const QDate checkOut = QDate::fromString(QString::fromStdString(booking->getEffectiveCheckOutDate()), Qt::ISODate);
        // Modified: Count operational check-ins only after the guest actually arrives, never from a merely scheduled reservation.
        const bool hasActuallyCheckedIn = state == BookingState::ACTIVE || state == BookingState::COMPLETED;
        if (hasActuallyCheckedIn && checkIn.isValid() && checkIn.month() == today.month() && checkIn.year() == today.year()) {
            ++report.bookingsThisMonth;
        }
        if (hasActuallyCheckedIn && checkIn.isValid() && checkIn.year() == today.year()) {
            ++report.bookingsThisYear;
        }

        // Modified: Rank rooms by actual arrivals so unarrived reservations and no-shows do not inflate demand reporting.
        if (hasActuallyCheckedIn && isInSelectedRange(checkIn, today, rangeIndex)) {
            const auto room = booking->getRoom();
            if (room && !room->isArchived()) {
                ++roomBookingCounts[room->getRoomNumber()];
            }
        }

        // Modified: Calculate selected-period occupancy from actual occupied room-nights, while retaining the live occupancy snapshot separately.
        if (hasActuallyCheckedIn && checkIn.isValid()) {
            const QDate actualEnd = state == BookingState::COMPLETED ? checkOut : today.addDays(1);
            const QDate overlapStart = std::max(checkIn, reportingWindow.start);
            const QDate overlapEnd = std::min(actualEnd, reportingWindow.endExclusive);
            if (overlapStart.isValid() && overlapEnd.isValid() && overlapEnd > overlapStart) {
                report.periodOccupiedRoomNights += overlapStart.daysTo(overlapEnd);
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
            invoice ? QString::fromStdString(invoice->getCustomerIdSnapshot()) : (customer ? QString::fromStdString(customer->getCustomerId()) : QStringLiteral("—")),
            invoice ? QString::fromStdString(invoice->getCustomerPhoneSnapshot()) : (customer ? QString::fromStdString(customer->getPhoneNumber()) : QStringLiteral("—")),
            invoice ? QString::fromStdString(invoice->getRoomNumberSnapshot()) : (room ? QString::fromStdString(room->getRoomNumber()) : QStringLiteral("—")),
            invoice ? QString::fromStdString(invoice->getRoomTypeSnapshot()) : (room ? QString::fromStdString(room->getRoomTypeName()) : QStringLiteral("—")),
            statusText(state),
            checkIn,
            checkOut
        };

        if (state == BookingState::COMPLETED && isInSelectedRange(checkOut, today, rangeIndex)) {
            report.completedStays.push_back(entry);
            if (invoice) {
                // Modified: Derive revenue KPIs from immutable invoices, not mutable room prices or a payment-status assumption.
                report.invoicedRevenue += invoice->calculateTotal();
                completedNightsInRange += invoice->getNights();
            }
        } else if ((state == BookingState::ACTIVE || state == BookingState::UPCOMING)
                   && isInSelectedRange(checkIn, today, rangeIndex)) {
            report.openBookings.push_back(entry);
        } else if (state == BookingState::CANCELLED && isInSelectedRange(checkIn, today, rangeIndex)) {
            report.cancelledBookings.push_back(entry);
        } else if (state == BookingState::NO_SHOW && isInSelectedRange(checkIn, today, rangeIndex)) {
            report.noShowBookings.push_back(entry);
        }
    }

    if (report.totalRooms > 0) {
        report.occupancyRate = (static_cast<double>(report.occupiedRooms) / report.totalRooms) * 100.0;
    }
    if (report.periodAvailableRoomNights > 0) {
        report.periodOccupancyRate = (static_cast<double>(report.periodOccupiedRoomNights)
                                      / report.periodAvailableRoomNights) * 100.0;
    }
    if (completedNightsInRange > 0) {
        report.averageDailyRate = report.invoicedRevenue / completedNightsInRange;
    }
    if (report.periodAvailableRoomNights > 0) {
        report.revenuePerAvailableRoom = report.invoicedRevenue / report.periodAvailableRoomNights;
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
    std::sort(report.noShowBookings.begin(), report.noShowBookings.end(), [](const ReportBookingEntry& left, const ReportBookingEntry& right) {
        return left.checkIn != right.checkIn ? left.checkIn < right.checkIn : left.bookingId < right.bookingId;
    });

    return report;
}
