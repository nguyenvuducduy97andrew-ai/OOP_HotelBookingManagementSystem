#pragma once

#include <QDate>
#include <QDateTime>
#include <QString>

#include <vector>

class HotelManager;

struct ReportRoomEntry
{
    QString roomNumber;
    QString type;
    int bookingCount = 0;
};

struct ReportBookingEntry
{
    QString bookingId;
    QString customerName;
    QString customerId;
    QString phone;
    QString roomNumber;
    QString roomType;
    QString status;
    QString operationalReason;
    QDateTime plannedCheckInAt;
    QDateTime plannedCheckOutAt;
    QDateTime actualCheckInAt;
    QDateTime actualCheckOutAt;
};

struct ReportOperationalBlockEntry
{
    QString roomNumber;
    QString roomType;
    QString blockType;
    QString status;
    QString note;
    QDateTime startsAt;
    QDateTime endsAt;
};

struct DashboardReportData
{
    QDateTime generatedAt;
    QString rangeLabel;
    QString rangeName;
    int totalRooms = 0;
    int standardRooms = 0;
    int deluxeRooms = 0;
    int suiteRooms = 0;
    int occupiedRooms = 0;
    int upcomingBookings = 0;
    int activeBookings = 0;
    int completedBookings = 0;
    int cancelledBookingsCount = 0;
    int bookingsThisMonth = 0;
    int bookingsThisYear = 0;
    double periodOccupiedRoomHours = 0.0;
    double periodCleaningRoomHours = 0.0;
    double periodMaintenanceRoomHours = 0.0;
    double periodSaleableRoomHours = 0.0;
    double occupancyRate = 0.0;
    double periodOccupancyRate = 0.0;
    double invoicedRevenue = 0.0;
    double averageBilledHourlyRate = 0.0;
    double revenuePerSaleableRoomHour = 0.0;
    std::vector<ReportRoomEntry> topRooms;
    std::vector<ReportBookingEntry> plannedArrivals;
    std::vector<ReportBookingEntry> plannedDepartures;
    std::vector<ReportOperationalBlockEntry> scheduledCleaning;
    std::vector<ReportOperationalBlockEntry> maintenanceWindows;
    std::vector<ReportBookingEntry> actualCheckIns;
    std::vector<ReportBookingEntry> actualCheckOuts;
    std::vector<ReportBookingEntry> cancelledBookings;
};

class ReportService
{
public:
    explicit ReportService(const HotelManager* hotelManager);

    DashboardReportData buildDashboardReport(int rangeIndex, const QString& rangeName) const;

private:
    const HotelManager* m_hotelManager;
};
