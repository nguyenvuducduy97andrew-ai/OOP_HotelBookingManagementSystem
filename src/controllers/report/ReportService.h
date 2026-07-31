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
    QDate checkIn;
    QDate checkOut;
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
    int noShowBookingsCount = 0;
    int bookingsThisMonth = 0;
    int bookingsThisYear = 0;
    int periodOccupiedRoomNights = 0;
    int periodAvailableRoomNights = 0;
    int periodDaysToDate = 0;
    double occupancyRate = 0.0;
    double periodOccupancyRate = 0.0;
    double invoicedRevenue = 0.0;
    double averageDailyRate = 0.0;
    double revenuePerAvailableRoom = 0.0;
    std::vector<ReportRoomEntry> topRooms;
    std::vector<ReportBookingEntry> openBookings;
    std::vector<ReportBookingEntry> completedStays;
    std::vector<ReportBookingEntry> cancelledBookings;
    std::vector<ReportBookingEntry> noShowBookings;
};

class ReportService
{
public:
    explicit ReportService(const HotelManager* hotelManager);

    DashboardReportData buildDashboardReport(int rangeIndex, const QString& rangeName) const;

private:
    const HotelManager* m_hotelManager;
};
