#include "DashboardWidget.h"
#include "ui_DashboardWidget.h"
#include "dashboardwidgets.h"
#include "HotelManager.h"
#include "StandardRoom.h"
#include "DeluxeRoom.h"
#include "SuiteRoom.h"

#include <QTimer>
#include <QDateTime>
#include <QtMath>
#include <QVBoxLayout>
#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QPdfWriter>
#include <QTextDocument>
#include <QTextBrowser>
#include <QTextStream>
#include <QFont>
#include <QPageSize>
#include <QPageLayout>
#include <QFileInfo>
#include <vector>
#include <map>
#include <unordered_set>
#include <algorithm>

namespace {
QString escapeHtml(const QString& text)
{
    return text.toHtmlEscaped();
}

struct CustomerAbuseRow
{
    QString customerId;
    QString customerName;
    QString phoneNumber;
    int totalBookings = 0;
    int cancelledBookings = 0;
    int deletedBookings = 0;

    int abuseScore() const
    {
        return cancelledBookings + deletedBookings;
    }
};
}

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QValueAxis>
#include <QtCharts/QBarCategoryAxis>



DashboardWidget::DashboardWidget(HotelManager *manager, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DashboardWidget)
    , m_manager(manager)
    , deletedBookingsBrowser(nullptr)
{
    ui->setupUi(this);

    // 1. Khởi tạo timer
    dateTimeTimer = new QTimer(this);

    // 2. Kết nối tín hiệu tới hàm thành viên (cách này sạch và dễ bảo trì)
    connect(dateTimeTimer, &QTimer::timeout, this, &DashboardWidget::updateDateTime);

    // 3. Khởi chạy
    dateTimeTimer->start(1000);

    // Gọi ngay một lần để hiển thị tức thì
    updateDateTime();

    // 4. Kết nối tín hiệu combobox lọc thời gian
    connect(ui->cmbDateRange, &QComboBox::currentIndexChanged, this, &DashboardWidget::refreshDashboard);
    connect(ui->btnExport, &QPushButton::clicked, this, &DashboardWidget::exportReport);

    if (ui->horizontalLayout) {
        ui->horizontalLayout->setAlignment(Qt::AlignVCenter);
    }
    if (ui->verticalLayoutTitle) {
        ui->verticalLayoutTitle->setContentsMargins(0, 0, 0, 0);
        ui->verticalLayoutTitle->setSpacing(4);
        ui->verticalLayoutTitle->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    }
    if (ui->verticalLayout) {
        ui->verticalLayout->setContentsMargins(0, 0, 0, 0);
        ui->verticalLayout->setSpacing(10);
        ui->verticalLayout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    }
    if (ui->mainVerticalLayout) {
        ui->mainVerticalLayout->setStretch(1, 1);
    }
    if (ui->bodyScrollArea) {
        ui->bodyScrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");
    }

    // ---- Phần mới: dựng nội dung dashboard ----
    populateData();
    buildTrendChart();
    buildBarChart();
    refreshDeletedBookingsView();
    applyStyle();

    auto *auditFrame = new QFrame(ui->scrollAreaWidgetContents);
    auditFrame->setObjectName("AuditCard");
    auto *auditLayout = new QVBoxLayout(auditFrame);
    auditLayout->setContentsMargins(18, 16, 18, 16);
    auditLayout->setSpacing(12);

    auto *auditTitle = new QLabel("Deleted Bookings History & Abuse Watchlist", auditFrame);
    auditTitle->setObjectName("SectionTitle");

    deletedBookingsBrowser = new QTextBrowser(auditFrame);
    deletedBookingsBrowser->setObjectName("AuditBrowser");
    deletedBookingsBrowser->setOpenExternalLinks(false);
    deletedBookingsBrowser->setFrameShape(QFrame::NoFrame);
    deletedBookingsBrowser->setMinimumHeight(220);
    deletedBookingsBrowser->setHtml(buildDeletedBookingsAuditHtml());

    auditLayout->addWidget(auditTitle);
    auditLayout->addWidget(deletedBookingsBrowser);

    if (ui->bodyLayout) {
        ui->bodyLayout->addWidget(auditFrame);
    }
}

void DashboardWidget::updateDateTime()
{
    const QString currentTime = QDateTime::currentDateTime().toString("dddd, dd/MM/yyyy · HH:mm:ss");
    ui->lblDate->setText(currentTime);
}

// =================================================================
void DashboardWidget::populateData()
{
    if (!m_manager) {
        ui->statCard1->setData("Total Room Number", "0", "0 room type", true);
        ui->statCard2->setData("Occupancy rate", "0%", "No data available", true);
        ui->statCard3->setData("This month reservation", "0", "No data available", true);
        ui->statCard4->setData("This year reservations", "0", "No data available", true);
        ui->miniCard1->setData("📅", QColor("#E8F0FF"), "Upcoming", "0");
        ui->miniCard2->setData("🛏", QColor("#E6FAF4"), "Active", "0");
        ui->miniCard3->setData("✔", QColor("#F0EBFF"), "Completed", "0");
        ui->miniCard4->setData("✖", QColor("#FDE8E6"), "Cancelled", "0");
        return;
    }

    int totalRooms = 0;
    int standardCount = 0;
    int deluxeCount = 0;
    int suiteCount = 0;
    int occupiedRooms = 0;
    int bookingsThisMonth = 0;
    int bookingsThisYear = 0;
    int upcomingCount = 0;
    int activeCount = 0;
    int completedCount = 0;
    int cancelledCount = 0;
    const QDate statsToday = QDate::currentDate();

    for (const auto& room : m_manager->getRooms()) {
        if (!room || room->isArchived()) {
            continue;
        }

        totalRooms++;
        const std::string typeName = room->getRoomTypeName();
        if (typeName == "Standard") {
            standardCount++;
        } else if (typeName == "Deluxe") {
            deluxeCount++;
        } else if (typeName == "Suite") {
            suiteCount++;
        }
    }

    std::unordered_set<std::string> occupiedRoomNumbers;
    for (const auto& booking : m_manager->getBookings()) {
        if (!booking || booking->isDeleted()) {
            continue;
        }

        const BookingState state = m_manager->getBookingState(*booking);
        switch (state) {
        case BookingState::UPCOMING: upcomingCount++; break;
        case BookingState::ACTIVE: activeCount++; break;
        case BookingState::COMPLETED: completedCount++; break;
        case BookingState::CANCELLED: cancelledCount++; break;
        }

        const QDate checkIn = QDate::fromString(QString::fromStdString(booking->getCheckInDate()), Qt::ISODate);
        if (!checkIn.isValid()) {
            continue;
        }

        if (checkIn.month() == statsToday.month() && checkIn.year() == statsToday.year()) {
            bookingsThisMonth++;
        }
        if (checkIn.year() == statsToday.year()) {
            bookingsThisYear++;
        }

        if (state == BookingState::ACTIVE) {
            const auto room = booking->getRoom();
            if (room && !room->isArchived()) {
                occupiedRoomNumbers.insert(room->getRoomNumber());
            }
        }
    }

    occupiedRooms = static_cast<int>(occupiedRoomNumbers.size());

    const int roomTypes = (standardCount > 0) + (deluxeCount > 0) + (suiteCount > 0);
    ui->statCard1->setData("Total Room Number", QString::number(totalRooms), QString::number(roomTypes) + " room types", true);

    double occupancyRate = 0.0;
    if (totalRooms > 0) {
        occupancyRate = (static_cast<double>(occupiedRooms) / static_cast<double>(totalRooms)) * 100.0;
    }
    ui->statCard2->setData("Occupancy rate", QString::number(qRound(occupancyRate)) + "%",
                           "Currently occupied " + QString::number(occupiedRooms) + "/" + QString::number(totalRooms) + " rooms", true);
    ui->statCard3->setData("This month's reservations", QString::number(bookingsThisMonth),
                           "Total reservations for " + QString::number(statsToday.month()), true);
    ui->statCard4->setData("This year reservations", QString::number(bookingsThisYear),
                           "Total reservations for " + QString::number(statsToday.year()), true);
    ui->miniCard1->setData("📅", QColor("#E8F0FF"), "Upcoming", QString::number(upcomingCount));
    ui->miniCard2->setData("🛏", QColor("#E6FAF4"), "Active", QString::number(activeCount));
    ui->miniCard3->setData("✔", QColor("#F0EBFF"), "Completed", QString::number(completedCount));
    ui->miniCard4->setData("✖", QColor("#FDE8E6"), "Cancelled", QString::number(cancelledCount));

    // ---- danh sách phòng nổi bật ----
    // 1. Cập nhật tiêu đề theo thời gian lọc
    int rangeIndex = ui->cmbDateRange->currentIndex();
    QString rangeText = "this month";
    if (rangeIndex == 0) rangeText = "today";
    else if (rangeIndex == 1) rangeText = "this week";
    else if (rangeIndex == 2) rangeText = "this month";
    else if (rangeIndex == 3) rangeText = "this year";

    ui->roomListTitleLabel->setText("Popular Rooms - " + rangeText);

    // 2. Tính số lượng đặt phòng cho từng phòng trong khoảng thời gian đã chọn
    std::map<std::string, int> roomBookingCounts;
    for (const auto& r : m_manager->getRooms()) {
        if (!r || r->isArchived()) continue;
        roomBookingCounts[r->getRoomNumber()] = 0;
    }

    for (const auto& b : m_manager->getBookings()) {
        if (!b || b->isCancelled() || b->isDeleted()) continue;
        
        QDate checkIn = QDate::fromString(QString::fromStdString(b->getCheckInDate()), Qt::ISODate);
        if (!checkIn.isValid()) continue;
        
        bool match = false;
        if (rangeIndex == 0) { // Hôm nay
            match = (checkIn == statsToday);
        } else if (rangeIndex == 1) { // Tuần này
            int checkInYear = 0;
            int checkInWeek = checkIn.weekNumber(&checkInYear);
            int todayYear = 0;
            int todayWeek = statsToday.weekNumber(&todayYear);
            match = (checkInWeek == todayWeek && checkInYear == todayYear);
        } else if (rangeIndex == 2) { // Tháng này
            match = (checkIn.month() == statsToday.month() && checkIn.year() == statsToday.year());
        } else if (rangeIndex == 3) { // Năm nay
            match = (checkIn.year() == statsToday.year());
        }
        
        if (match) {
            auto room = b->getRoom();
            if (room && !room->isArchived()) {
                roomBookingCounts[room->getRoomNumber()]++;
            }
        }
    }

    // 3. Sắp xếp các phòng theo số lượt đặt giảm dần
    struct RoomStats {
        std::shared_ptr<Room> room;
        int bookingCount;
    };
    std::vector<RoomStats> roomStatsList;
    for (const auto& r : m_manager->getRooms()) {
        if (!r || r->isArchived()) continue;
        roomStatsList.push_back({r, roomBookingCounts[r->getRoomNumber()]});
    }

    std::sort(roomStatsList.begin(), roomStatsList.end(), [](const RoomStats& a, const RoomStats& b) {
        if (a.bookingCount != b.bookingCount) {
            return a.bookingCount > b.bookingCount; // Nhiều nhất lên trước
        }
        return a.room->getRoomNumber() < b.room->getRoomNumber(); // Trùng thì xếp theo số phòng tăng dần
    });

    // 4. Hiển thị top 3 phòng nổi bật
    int numToShow = std::min(3, (int)roomStatsList.size());
    ui->roomItem1->setVisible(numToShow >= 1);
    ui->roomItem2->setVisible(numToShow >= 2);
    ui->roomItem3->setVisible(numToShow >= 3);

    for (int i = 0; i < numToShow; ++i) {
        auto room = roomStatsList[i].room;
        int count = roomStatsList[i].bookingCount;
        
        // Fixed-modified: Use the room's virtual type name for summary cards.
        QString typeLabel = QString::fromStdString(room->getRoomTypeName());
        
        QString roomTitle = QString("Room %1 · %2")
            .arg(QString::fromStdString(room->getRoomNumber()))
            .arg(typeLabel);
            
        QString subtitle;
        QColor badgeColor = QColor("#05CD99"); // Màu xanh lá mặc định
        
        if (i == 0) {
            subtitle = "Most booked";
        } else if (i == 1) {
            if (numToShow == 2) {
                subtitle = "Least booked";
                badgeColor = QColor("#EE5D50"); // Đỏ
            } else {
                subtitle = "Second most booked";
            }
        } else if (i == 2) {
            bool isLeast = (roomStatsList.size() == 3) || (count == roomStatsList.back().bookingCount);
            if (isLeast) {
                subtitle = "Least booked";
                badgeColor = QColor("#EE5D50"); // Đỏ
            } else {
                subtitle = "Third most booked";
                badgeColor = QColor("#005BFE"); // Xanh dương
            }
        }
        
        QString badgeText = QString("%1 bookings").arg(count);
        
        if (i == 0) {
            ui->roomItem1->setData(roomTitle, subtitle, badgeText, badgeColor);
        } else if (i == 1) {
            ui->roomItem2->setData(roomTitle, subtitle, badgeText, badgeColor);
        } else if (i == 2) {
            ui->roomItem3->setData(roomTitle, subtitle, badgeText, badgeColor);
        }
    }
}

// =================================================================
void DashboardWidget::buildTrendChart()
{
    int currentYear = QDate::currentDate().year();
    int prevYear = currentYear - 1;

    QVector<double> dataCurrent(12, 0.0);
    QVector<double> dataPrev(12, 0.0);

    if (m_manager) {
        for (const auto& b : m_manager->getBookings()) {
            if (!b || b->isDeleted()) continue;
            QDate checkIn = QDate::fromString(QString::fromStdString(b->getCheckInDate()), Qt::ISODate);
            if (checkIn.isValid()) {
                int m = checkIn.month() - 1; // 0-11
                if (checkIn.year() == currentYear) {
                    dataCurrent[m] += 1.0;
                } else if (checkIn.year() == prevYear) {
                    dataPrev[m] += 1.0;
                }
            }
        }
    }

    QStringList months = {"Jan","Feb","Mar","Apr","May","Jun","Jul",
                          "Aug","Sep","Oct","Nov","Dec"};

    auto *seriesCurrent = new QLineSeries();
    seriesCurrent->setName(QString::number(currentYear));
    seriesCurrent->setColor(QColor("#005BFE"));
    for (int i = 0; i < 12; ++i)
        seriesCurrent->append(i, dataCurrent[i]);

    auto *seriesPrev = new QLineSeries();
    seriesPrev->setName(QString::number(prevYear));
    QPen dashedPen(QColor("#CBD5E0"));
    dashedPen.setStyle(Qt::DashLine);
    dashedPen.setWidth(2);
    seriesPrev->setPen(dashedPen);
    for (int i = 0; i < 12; ++i)
        seriesPrev->append(i, dataPrev[i]);

    auto *chart = new QChart();
    chart->addSeries(seriesCurrent);
    chart->addSeries(seriesPrev);
    chart->legend()->setVisible(true);
    chart->legend()->setLabelColor(QColor("#2B3674"));
    chart->legend()->setAlignment(Qt::AlignTop);
    chart->setBackgroundBrush(QColor("#FFFFFF"));
    chart->setMargins(QMargins(4, 4, 4, 4));

    auto *axisXCat = new QBarCategoryAxis();
    axisXCat->append(months);
    axisXCat->setLabelsColor(QColor("#A3AED0"));
    axisXCat->setGridLineColor(QColor("#F1F5F9"));

    // Find the max booking count in a month to dynamically scale axis Y
    double maxVal = 10.0;
    for (double val : dataCurrent) {
        if (val > maxVal) maxVal = val;
    }
    for (double val : dataPrev) {
        if (val > maxVal) maxVal = val;
    }
    maxVal = qCeil(maxVal / 5.0) * 5.0;

    auto *axisY = new QValueAxis();
    axisY->setRange(0, maxVal);
    axisY->setTickCount(6);
    axisY->setLabelsColor(QColor("#A3AED0"));
    axisY->setGridLineColor(QColor("#F1F5F9"));

    chart->addAxis(axisXCat, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    seriesCurrent->attachAxis(axisXCat);
    seriesCurrent->attachAxis(axisY);
    seriesPrev->attachAxis(axisXCat);
    seriesPrev->attachAxis(axisY);

    auto *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setStyleSheet("background: transparent;");

    QLayout* oldLayout = ui->trendChartHost->layout();
    if (oldLayout) {
        QLayoutItem* item;
        while ((item = oldLayout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        oldLayout->addWidget(chartView);
    } else {
        auto *hostLayout = new QVBoxLayout(ui->trendChartHost);
        hostLayout->setContentsMargins(0, 0, 0, 0);
        hostLayout->addWidget(chartView);
    }
}

// =================================================================
void DashboardWidget::buildBarChart()
{
    int standardBookings = 0;
    int deluxeBookings = 0;
    int suiteBookings = 0;

    if (m_manager) {
        for (const auto& b : m_manager->getBookings()) {
            if (!b || b->isDeleted()) continue;
            auto room = b->getRoom();
            if (!room) continue;
            if (dynamic_cast<StandardRoom*>(room.get())) {
                standardBookings++;
            } else if (dynamic_cast<DeluxeRoom*>(room.get())) {
                deluxeBookings++;
            } else if (dynamic_cast<SuiteRoom*>(room.get())) {
                suiteBookings++;
            }
        }
    }

    auto *set = new QBarSet("Số lượng");
    set->append({(double)standardBookings, (double)deluxeBookings, (double)suiteBookings});
    set->setColor(QColor("#005BFE"));
    set->setBorderColor(Qt::transparent);

    auto *series = new QBarSeries();
    series->append(set);
    series->setBarWidth(0.4);

    auto *chart = new QChart();
    chart->addSeries(series);
    chart->legend()->setVisible(false);
    chart->setBackgroundBrush(QColor("#FFFFFF"));
    chart->setMargins(QMargins(4, 4, 4, 4));

    QStringList categories = {"Standard", "Deluxe", "Suite"};
    auto *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setLabelsColor(QColor("#A3AED0"));
    axisX->setGridLineColor(QColor("#F1F5F9"));

    // Find the max booking count in room types to scale Y axis dynamically
    double maxVal = 10.0;
    if (standardBookings > maxVal) maxVal = standardBookings;
    if (deluxeBookings > maxVal) maxVal = deluxeBookings;
    if (suiteBookings > maxVal) maxVal = suiteBookings;
    maxVal = qCeil(maxVal / 5.0) * 5.0;

    auto *axisY = new QValueAxis();
    axisY->setRange(0, maxVal);
    axisY->setTickCount(6);
    axisY->setLabelsColor(QColor("#A3AED0"));
    axisY->setGridLineColor(QColor("#F1F5F9"));

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisX);
    series->attachAxis(axisY);

    auto *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setStyleSheet("background: transparent;");

    QLayout* oldLayout = ui->barChartHost->layout();
    if (oldLayout) {
        QLayoutItem* item;
        while ((item = oldLayout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        oldLayout->addWidget(chartView);
    } else {
        auto *hostLayout = new QVBoxLayout(ui->barChartHost);
        hostLayout->setContentsMargins(0, 0, 0, 0);
        hostLayout->addWidget(chartView);
    }
}

// =================================================================
void DashboardWidget::applyStyle()
{
    setStyleSheet(R"(
        DashboardWidget {
            background-color: #F4F7FE;
        }
        #HeaderFrame {
            background-color: #FFFFFF;
            border: 1px solid #E2E8F0;
            border-radius: 16px;
        }
        QLabel#lblTitle {
            font-size: 22px;
            font-weight: 700;
            color: #1B2559;
            letter-spacing: -0.3px;
        }
        QLabel#lblDate {
            font-size: 12px;
            color: #8F9BB7;
        }
        QComboBox#cmbDateRange {
            font: 11pt "Segoe UI";
            border: 1px solid #E2E8F0;
            border-radius: 10px;
            padding: 6px 12px;
            background-color: #FFFFFF;
            color: #2B3674;
        }
        QComboBox#cmbDateRange:hover {
            border-color: #CBD5E1;
        }
        QComboBox#cmbDateRange::drop-down {
            border: none;
            width: 24px;
        }
        QComboBox#cmbDateRange QAbstractItemView {
            border: 1px solid #E2E8F0;
            background-color: #FFFFFF;
            color: #2B3674;
            selection-background-color: #005BFE;
            selection-color: #FFFFFF;
            outline: none;
            padding: 4px;
        }
        QPushButton#btnExport {
            background-color: #005BFE;
            color: #FFFFFF;
            border: none;
            border-radius: 10px;
            padding: 8px 16px;
            font-size: 11pt;
            font-weight: 600;
        }
        QPushButton#btnExport:hover {
            background-color: #2B7BFF;
        }
        QPushButton#btnExport:pressed {
            background-color: #0046CC;
        }
        StatCard, MiniCard, #ChartCard, #AuditCard {
            background-color: #FFFFFF;
            border: 1px solid #E2E8F0;
            border-radius: 16px;
        }
        #statCard1 {
            border-top: 3px solid #005BFE;
        }
        #statCard2 {
            border-top: 3px solid #05CD99;
        }
        #statCard3 {
            border-top: 3px solid #FFB547;
        }
        #statCard4 {
            border-top: 3px solid #7551FF;
        }
        #CardTitle {
            font-size: 12px;
            font-weight: 600;
            color: #8F9BB7;
        }
        #CardValue {
            font-size: 28px;
            font-weight: 700;
            color: #1B2559;
        }
        #CardSubtitlePositive {
            font-size: 11px;
            color: #05CD99;
            font-weight: 600;
        }
        #CardSubtitleNegative {
            font-size: 11px;
            color: #EE5D50;
            font-weight: 600;
        }
        #MiniCardLabel {
            font-size: 12px;
            color: #8F9BB7;
            font-weight: 500;
        }
        #MiniCardValue {
            font-size: 18px;
            font-weight: 700;
            color: #1B2559;
        }
        #ChartTitle, #SectionTitle {
            font-size: 15px;
            font-weight: 700;
            color: #1B2559;
        }
        #RoomListItem {
            background-color: #F8FAFC;
            border: 1px solid #EEF2F7;
            border-radius: 12px;
        }
        #RoomTitle {
            font-size: 13px;
            font-weight: 700;
            color: #1B2559;
        }
        #RoomSubtitle {
            font-size: 11px;
            color: #8F9BB7;
        }
        QTextBrowser#AuditBrowser {
            background-color: #F8FAFC;
            border: 1px solid #EEF2F7;
            border-radius: 12px;
            padding: 12px;
        }
    )");
}

void DashboardWidget::refreshDashboard() {
    updateDateTime();
    populateData();
    buildTrendChart();
    buildBarChart();
    refreshDeletedBookingsView();
}

void DashboardWidget::refreshDeletedBookingsView()
{
    if (deletedBookingsBrowser) {
        deletedBookingsBrowser->setHtml(buildDeletedBookingsAuditHtml());
    }
}

QString DashboardWidget::buildDeletedBookingsAuditHtml() const
{
    QString html;
    QTextStream stream(&html);
    stream << "<style>"
           << "body{font-family:Segoe UI,Arial,sans-serif;color:#334155;margin:0;padding:0;font-size:11pt;line-height:1.5;}"
           << "h3{color:#1B2559;margin:0 0 8px 0;font-size:13pt;font-weight:700;}"
           << "p{margin:0;color:#64748B;}"
           << "table{border-collapse:collapse;width:100%;margin-top:10px;border-radius:8px;overflow:hidden;}"
           << "th,td{border:1px solid #E2E8F0;padding:10px 12px;text-align:left;vertical-align:top;}"
           << "th{background:#F1F5F9;color:#1B2559;font-size:10pt;}"
           << "tr:nth-child(even){background:#FAFBFC;}"
           << ".section{background:#FFFFFF;border:1px solid #EEF2F7;border-radius:12px;padding:16px;margin-bottom:12px;}"
           << ".empty{display:flex;flex-direction:column;align-items:center;text-align:center;padding:20px 12px;color:#94A3B8;}"
           << ".empty-icon{font-size:28px;margin-bottom:8px;opacity:0.7;}"
           << ".empty-title{font-size:12pt;font-weight:600;color:#64748B;margin-bottom:4px;}"
           << ".note{font-size:10pt;color:#94A3B8;}"
           << ".warn{color:#B45309;font-weight:600;}"
           << ".muted{color:#94A3B8;}"
           << "</style>";

    stream << "<div class='section'>";
    stream << "<h3>Deleted Bookings History</h3>";

    if (!m_manager) {
        stream << "<div class='empty'><div class='empty-icon'>&#128196;</div>"
               << "<div class='empty-title'>No data available</div>"
               << "<p class='note'>Connect the hotel manager to view audit records.</p></div>";
        stream << "</div>";
        return html;
    }

    struct DeletedBookingRow {
        QString bookingId;
        QString customerId;
        QString customerName;
        QString roomNumber;
        QString checkIn;
        QString checkOut;
        QString state;
    };

    std::vector<DeletedBookingRow> deletedRows;
    std::map<std::string, CustomerAbuseRow> customerRows;

    for (const auto& booking : m_manager->getBookings()) {
        if (!booking) {
            continue;
        }

        const auto customer = booking->getCustomer();
        const auto room = booking->getRoom();
        if (!customer) {
            continue;
        }

        auto &customerRow = customerRows[customer->getCustomerId()];
        customerRow.customerId = QString::fromStdString(customer->getCustomerId());
        customerRow.customerName = QString::fromStdString(customer->getName());
        customerRow.phoneNumber = QString::fromStdString(customer->getPhoneNumber());
        customerRow.totalBookings++;
        if (booking->isCancelled()) {
            customerRow.cancelledBookings++;
        }
        if (booking->isDeleted()) {
            customerRow.deletedBookings++;
        }

        if (booking->isDeleted()) {
            deletedRows.push_back({
                QString::fromStdString(booking->getBookingId()),
                QString::fromStdString(customer->getCustomerId()),
                QString::fromStdString(customer->getName()),
                room ? QString::fromStdString(room->getRoomNumber()) : QString("-") ,
                QString::fromStdString(booking->getCheckInDate()),
                QString::fromStdString(booking->getCheckOutDate()),
                QString::fromStdString(bookingStateToString(m_manager->getBookingState(*booking)))
            });
        }
    }

    if (deletedRows.empty()) {
        stream << "<div class='empty'><div class='empty-icon'>&#9989;</div>"
               << "<div class='empty-title'>No deleted bookings yet</div>"
               << "<p class='note'>Deleted records will appear here for audit and abuse review.</p></div>";
    } else {
        stream << "<table><tr><th>Booking ID</th><th>Customer</th><th>Room</th><th>Check-in</th><th>Check-out</th><th>Status</th></tr>";
        for (const auto& row : deletedRows) {
            stream << "<tr><td>" << escapeHtml(row.bookingId) << "</td><td>" << escapeHtml(row.customerName) << " (" << escapeHtml(row.customerId) << ")</td><td>" << escapeHtml(row.roomNumber) << "</td><td>" << escapeHtml(row.checkIn) << "</td><td>" << escapeHtml(row.checkOut) << "</td><td class='warn'>" << escapeHtml(row.state) << "</td></tr>";
        }
        stream << "</table>";
    }
    stream << "</div>";

    std::vector<CustomerAbuseRow> abuseRows;
    for (const auto& [customerId, row] : customerRows) {
        if (row.abuseScore() >= 2) {
            abuseRows.push_back(row);
        }
    }

    std::sort(abuseRows.begin(), abuseRows.end(), [](const CustomerAbuseRow& a, const CustomerAbuseRow& b) {
        if (a.abuseScore() != b.abuseScore()) {
            return a.abuseScore() > b.abuseScore();
        }
        if (a.deletedBookings != b.deletedBookings) {
            return a.deletedBookings > b.deletedBookings;
        }
        return a.customerId < b.customerId;
    });

    stream << "<div class='section'>";
    stream << "<h3>Customer Cancel/Delete Abuse Watchlist</h3>";
    stream << "<p class='note'>Customers with 2 or more combined cancel/delete actions are flagged below.</p>";

    if (abuseRows.empty()) {
        stream << "<div class='empty'><div class='empty-icon'>&#128737;</div>"
               << "<div class='empty-title'>All clear</div>"
               << "<p class='note'>No suspicious customer patterns found yet.</p></div>";
    } else {
        stream << "<table><tr><th>Customer</th><th>Total Bookings</th><th>Cancelled</th><th>Deleted</th><th>Action Score</th><th>Contact</th></tr>";
        for (const auto& row : abuseRows) {
            const bool highRisk = row.abuseScore() >= 4;
            stream << "<tr><td>" << escapeHtml(row.customerName) << " (" << escapeHtml(row.customerId) << ")</td><td>" << row.totalBookings << "</td><td>" << row.cancelledBookings << "</td><td>" << row.deletedBookings << "</td><td class='" << (highRisk ? "warn" : "") << "'>" << row.abuseScore() << "</td><td>" << escapeHtml(row.phoneNumber) << "</td></tr>";
        }
        stream << "</table>";
    }
    stream << "</div>";

    return html;
}

QString DashboardWidget::buildReportHtml() const
{
    const QDateTime now = QDateTime::currentDateTime();
    const QDate today = now.date();
    const int rangeIndex = ui->cmbDateRange->currentIndex();

    int totalRooms = 0;
    int standardCount = 0;
    int deluxeCount = 0;
    int suiteCount = 0;
    int occupiedRooms = 0;
    int upcomingCount = 0;
    int activeCount = 0;
    int completedCount = 0;
    int cancelledCount = 0;
    int bookingsThisMonth = 0;
    int bookingsThisYear = 0;

    struct RoomEntry {
        QString roomNumber;
        QString type;
        int bookingCount = 0;
    };

    std::map<std::string, int> roomBookingCounts;
    if (m_manager) {
        // Fixed-modified: Count room categories through the virtual type accessor.
        for (const auto& room : m_manager->getRooms()) {
            if (!room || room->isArchived()) {
                continue;
            }

            totalRooms++;
            const std::string typeName = room->getRoomTypeName();
            if (typeName == "Standard") {
                standardCount++;
            } else if (typeName == "Deluxe") {
                deluxeCount++;
            } else if (typeName == "Suite") {
                suiteCount++;
            }

            roomBookingCounts[room->getRoomNumber()] = 0;
        }

        occupiedRooms = static_cast<int>(m_manager->getRoomsByOccupancy(true).size());

        for (const auto& booking : m_manager->getBookings()) {
            if (!booking || booking->isDeleted()) {
                continue;
            }

            const BookingState state = m_manager->getBookingState(*booking);
            switch (state) {
            case BookingState::UPCOMING: upcomingCount++; break;
            case BookingState::ACTIVE: activeCount++; break;
            case BookingState::COMPLETED: completedCount++; break;
            case BookingState::CANCELLED: cancelledCount++; break;
            }

            const QDate checkIn = QDate::fromString(QString::fromStdString(booking->getCheckInDate()), Qt::ISODate);
            if (checkIn.isValid() && checkIn.month() == today.month() && checkIn.year() == today.year()) {
                bookingsThisMonth++;
            }
            if (checkIn.isValid() && checkIn.year() == today.year()) {
                bookingsThisYear++;
            }

            bool match = false;
            if (rangeIndex == 0) {
                match = (checkIn == today);
            } else if (rangeIndex == 1) {
                int checkInYear = 0;
                int checkInWeek = checkIn.weekNumber(&checkInYear);
                int todayYear = 0;
                int todayWeek = today.weekNumber(&todayYear);
                match = (checkInWeek == todayWeek && checkInYear == todayYear);
            } else if (rangeIndex == 2) {
                match = (checkIn.month() == today.month() && checkIn.year() == today.year());
            } else if (rangeIndex == 3) {
                match = (checkIn.year() == today.year());
            }

            if (match && !booking->isCancelled()) {
                const auto room = booking->getRoom();
                if (room && !room->isArchived()) {
                    roomBookingCounts[room->getRoomNumber()]++;
                }
            }
        }
    }

    double occupancyRate = 0.0;
    if (totalRooms > 0) {
        occupancyRate = (static_cast<double>(occupiedRooms) / static_cast<double>(totalRooms)) * 100.0;
    }

    std::vector<RoomEntry> popularRooms;
    for (const auto& [roomNumber, count] : roomBookingCounts) {
        // Fixed-modified: Read the room type label without using dynamic_cast.
        QString typeLabel = "Standard";
        if (m_manager) {
            const auto room = m_manager->findRoomByNumber(roomNumber);
            if (room) {
                typeLabel = QString::fromStdString(room->getRoomTypeName());
            }
        }

        popularRooms.push_back({QString::fromStdString(roomNumber), typeLabel, count});
    }

    std::sort(popularRooms.begin(), popularRooms.end(), [](const RoomEntry& a, const RoomEntry& b) {
        if (a.bookingCount != b.bookingCount) {
            return a.bookingCount > b.bookingCount;
        }
        return a.roomNumber < b.roomNumber;
    });

    QString rangeLabel = "today";
    if (rangeIndex == 1) {
        rangeLabel = "this week";
    } else if (rangeIndex == 2) {
        rangeLabel = "this month";
    } else if (rangeIndex == 3) {
        rangeLabel = "this year";
    }

    QString html;
    QTextStream stream(&html);
    stream << "<html><head><meta charset='utf-8'>"
           << "<style>"
            << "body{font-family:Segoe UI,Arial,sans-serif;color:#1f2937;padding:18px;font-size:13pt;line-height:1.45;}"
            << "h1{color:#2B3674;margin:0 0 10px 0;font-size:28pt;}"
            << "h2{color:#2B3674;margin:24px 0 12px 0;font-size:17pt;}"
            << "table{border-collapse:collapse;width:100%;margin-top:12px;}"
            << "th,td{border:1px solid #e5e7eb;padding:12px 14px;text-align:left;vertical-align:top;font-size:12.5pt;}"
           << "th{background:#f8fafc;color:#2B3674;}"
            << ".meta{color:#6b7280;margin-bottom:20px;font-size:11.5pt;}"
            << ".summary{border-collapse:separate;border-spacing:12px 0;margin-top:12px;}"
            << ".summary td{border:1px solid #e5e7eb;border-radius:12px;padding:18px 20px;background:#fff;width:25%;}"
            << ".label{font-size:11pt;color:#94a3b8;margin-bottom:6px;}"
            << ".value{font-size:24pt;font-weight:700;color:#2B3674;margin-top:6px;}"
            << ".sectionNote{font-size:11pt;color:#94a3b8;margin-top:4px;}"
           << "</style></head><body>";

    stream << "<h1>Booking Management Dashboard Report</h1>";
    stream << "<div class='meta'>Generated at: " << escapeHtml(now.toString("dd/MM/yyyy HH:mm:ss")) << "</div>";
    stream << "<div class='sectionNote'>Selected range: " << escapeHtml(rangeLabel) << "</div>";

    stream << "<table class='summary'><tr>";
    stream << "<td><div class='label'>Total rooms</div><div class='value'>" << totalRooms << "</div></td>";
    stream << "<td><div class='label'>Room types</div><div class='value'>" << ((standardCount > 0) + (deluxeCount > 0) + (suiteCount > 0)) << "</div></td>";
    stream << "<td><div class='label'>Occupancy rate</div><div class='value'>" << qRound(occupancyRate) << "%</div></td>";
    stream << "<td><div class='label'>Active bookings</div><div class='value'>" << activeCount << "</div></td>";
    stream << "</tr></table>";

    stream << "<h2>Booking status</h2>";
    stream << "<table><tr><th>Upcoming</th><th>Active</th><th>Completed</th><th>Cancelled</th><th>This month</th><th>This year</th></tr>";
    stream << "<tr><td>" << upcomingCount << "</td><td>" << activeCount << "</td><td>" << completedCount << "</td><td>" << cancelledCount << "</td><td>" << bookingsThisMonth << "</td><td>" << bookingsThisYear << "</td></tr></table>";

    stream << "<h2>Rooms by type</h2>";
    stream << "<table><tr><th>Standard</th><th>Deluxe</th><th>Suite</th></tr>";
    stream << "<tr><td>" << standardCount << "</td><td>" << deluxeCount << "</td><td>" << suiteCount << "</td></tr></table>";

    stream << "<h2>Popular rooms (" << escapeHtml(rangeLabel) << ")</h2>";
    stream << "<table><tr><th>#</th><th>Room</th><th>Type</th><th>Bookings</th></tr>";
    const int topCount = std::min(3, static_cast<int>(popularRooms.size()));
    for (int i = 0; i < topCount; ++i) {
        const auto& item = popularRooms[i];
        stream << "<tr><td>" << (i + 1) << "</td><td>" << escapeHtml(item.roomNumber) << "</td><td>" << escapeHtml(item.type) << "</td><td>" << item.bookingCount << "</td></tr>";
    }
    stream << "</table>";

    stream << "<h2>Deleted bookings history & abuse watchlist</h2>";
    stream << buildDeletedBookingsAuditHtml();

    stream << "</body></html>";
    return html;
}

void DashboardWidget::exportReport()
{
    const QString defaultName = QString("dashboard_report_%1.pdf").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    const QString filePath = QFileDialog::getSaveFileName(
        this,
        "Export Dashboard Report",
        defaultName,
        "PDF Report (*.pdf)"
    );

    if (filePath.isEmpty()) {
        return;
    }

    QString finalPath = filePath;
    if (!finalPath.endsWith(".pdf", Qt::CaseInsensitive)) {
        finalPath += ".pdf";
    }

    QPdfWriter writer(finalPath);
    QPageLayout pageLayout(QPageSize(QPageSize::A4), QPageLayout::Landscape, QMarginsF(10, 10, 10, 10));
    writer.setPageLayout(pageLayout);
    writer.setResolution(300);

    QTextDocument document;
    document.setDefaultFont(QFont("Segoe UI", 12));
    document.setDocumentMargin(0);
    document.setHtml(buildReportHtml());
    document.setPageSize(QSizeF(pageLayout.paintRectPoints().size()));

    document.print(&writer);

    if (QFileInfo::exists(finalPath)) {
        QMessageBox::information(this, "Export Report", "Dashboard report exported successfully as PDF.");
        return;
    }

    QMessageBox::critical(this, "Export Error", "Cannot write the PDF report file.");
}

DashboardWidget::~DashboardWidget()
{
    delete ui;
}
