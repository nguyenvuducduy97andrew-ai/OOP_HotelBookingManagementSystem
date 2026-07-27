#include "DashboardWidget.h"
#include "ui_DashboardWidget.h"
#include "dashboardwidgets.h"
#include "HotelManager.h"
#include "ReportService.h"
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
#include <QScrollBar>
#include <QUrl>
#include <vector>
#include <map>
#include <unordered_set>
#include <algorithm>

namespace {
QString escapeHtml(const QString& text)
{
    return text.toHtmlEscaped();
}

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
    , bookingHistoryBrowser(nullptr)
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
    // Modified and optimized performance: reset history pagination only when its selected time range changes.
    connect(ui->cmbDateRange, &QComboBox::currentIndexChanged, this, [this](int) {
        m_historyPage = 0;
        refreshDashboard();
    });
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
        ui->bodyScrollArea->verticalScrollBar()->setSingleStep(18);
    }

    // ---- Phần mới: dựng nội dung dashboard ----
    populateData();
    buildTrendChart();
    buildBarChart();
    refreshBookingHistoryView();
    applyStyle();

    auto *auditFrame = new QFrame(ui->scrollAreaWidgetContents);
    auditFrame->setObjectName("AuditCard");
    auto *auditLayout = new QVBoxLayout(auditFrame);
    auditLayout->setContentsMargins(18, 16, 18, 16);
    auditLayout->setSpacing(12);

    auto *auditTitle = new QLabel("Booking History", auditFrame);
    auditTitle->setObjectName("SectionTitle");

    bookingHistoryBrowser = new QTextBrowser(auditFrame);
    bookingHistoryBrowser->setObjectName("AuditBrowser");
    bookingHistoryBrowser->setOpenExternalLinks(false);
    // Modified and optimized performance: handle local page links without navigating QTextBrowser away from the history view.
    bookingHistoryBrowser->setOpenLinks(false);
    bookingHistoryBrowser->setFrameShape(QFrame::NoFrame);
    bookingHistoryBrowser->setMinimumHeight(170);
    bookingHistoryBrowser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    bookingHistoryBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    bookingHistoryBrowser->document()->setDocumentMargin(0);
    connect(bookingHistoryBrowser, &QTextBrowser::anchorClicked,
            this, &DashboardWidget::onBookingHistoryLinkClicked);

    auditLayout->addWidget(auditTitle);
    auditLayout->addWidget(bookingHistoryBrowser);

    if (ui->bodyLayout) {
        ui->bodyLayout->addWidget(auditFrame);
    }
    QTimer::singleShot(0, this, &DashboardWidget::refreshBookingHistoryView);
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
            border: 1px solid #E8EEF6;
            border-radius: 14px;
            padding: 10px;
            color: #475569;
        }
        QTextBrowser#AuditBrowser QScrollBar:vertical {
            background: transparent;
            width: 10px;
            margin: 10px 3px 10px 0;
        }
        QTextBrowser#AuditBrowser QScrollBar::handle:vertical {
            background: #CBD5E1;
            border-radius: 4px;
            min-height: 36px;
        }
        QTextBrowser#AuditBrowser QScrollBar::handle:vertical:hover {
            background: #94A3B8;
        }
        QTextBrowser#AuditBrowser QScrollBar::add-line:vertical,
        QTextBrowser#AuditBrowser QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QTextBrowser#AuditBrowser QScrollBar::add-page:vertical,
        QTextBrowser#AuditBrowser QScrollBar::sub-page:vertical {
            background: transparent;
        }
        QScrollArea#bodyScrollArea {
            background: transparent;
            border: none;
        }
        QScrollArea#bodyScrollArea QScrollBar:vertical {
            background: transparent;
            width: 12px;
            margin: 8px 3px 8px 3px;
        }
        QScrollArea#bodyScrollArea QScrollBar::handle:vertical {
            background: #C7D3E3;
            border: 3px solid transparent;
            border-radius: 6px;
            min-height: 48px;
        }
        QScrollArea#bodyScrollArea QScrollBar::handle:vertical:hover {
            background: #94A9C2;
        }
        QScrollArea#bodyScrollArea QScrollBar::add-line:vertical,
        QScrollArea#bodyScrollArea QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QScrollArea#bodyScrollArea QScrollBar::add-page:vertical,
        QScrollArea#bodyScrollArea QScrollBar::sub-page:vertical {
            background: transparent;
        }
    )");
}

void DashboardWidget::refreshDashboard() {
    updateDateTime();
    populateData();
    buildTrendChart();
    buildBarChart();
    refreshBookingHistoryView();
}

void DashboardWidget::refreshBookingHistoryView()
{
    if (bookingHistoryBrowser) {
        bookingHistoryBrowser->setHtml(buildBookingHistoryHtml());
        // Modified and optimized performance: size the browser to its current page so the dashboard owns the main scrollbar.
        bookingHistoryBrowser->document()->setTextWidth(bookingHistoryBrowser->viewport()->width());
        const int contentHeight = qCeil(bookingHistoryBrowser->document()->size().height()) + 20;
        bookingHistoryBrowser->setFixedHeight(qMax(170, contentHeight));
    }
}

void DashboardWidget::onBookingHistoryLinkClicked(const QUrl& url)
{
    if (url.scheme() != "page") {
        return;
    }

    bool ok = false;
    const int page = url.path().toInt(&ok);
    if (ok && page >= 0) {
        m_historyPage = page;
        refreshBookingHistoryView();
    }
}

QString DashboardWidget::buildBookingHistoryHtml() const
{
    QString html;
    QTextStream stream(&html);
    stream << "<style>"
           << "body{font-family:'Segoe UI',Arial,sans-serif;color:#334155;margin:0;padding:0;font-size:10pt;line-height:1.45;background:#F8FAFC;}"
           << ".audit-section{background:#FFFFFF;border:1px solid #E8EEF6;border-radius:10px;padding:14px;}"
           << ".section-header{width:100%;border-collapse:collapse;margin:0 0 12px 0;}"
           << ".kicker{font-size:8pt;font-weight:700;letter-spacing:1px;color:#8F9BB7;margin-bottom:3px;}"
           << ".section-title{font-size:13pt;font-weight:700;color:#1B2559;margin-bottom:3px;}"
           << ".section-description{font-size:9pt;color:#8F9BB7;}"
           << ".history-count{font-size:16pt;font-weight:800;color:#2B6DEF;margin-top:7px;}"
           << ".badge{background:#EEF4FF;color:#2B6DEF;border:1px solid #DCE8FF;border-radius:9px;padding:4px 8px;font-size:8pt;font-weight:700;}"
           << ".badge-clear{background:#EAFBF5;color:#099268;border:1px solid #C9F2E3;border-radius:9px;padding:4px 8px;font-size:8pt;font-weight:700;}"
           << ".empty-state{background:#F8FAFC;border:1px dashed #D9E3EF;border-radius:9px;padding:15px 16px;text-align:center;}"
           << ".state-mark{font-size:18pt;font-weight:700;color:#2B7BFF;margin-bottom:3px;}"
           << ".state-mark-clear{font-size:18pt;font-weight:700;color:#05A97B;margin-bottom:3px;}"
           << ".empty-title{font-size:11pt;font-weight:700;color:#2B3674;margin-bottom:3px;}"
           << ".note{font-size:9pt;color:#8F9BB7;}"
           << ".history-row{width:100%;border-collapse:collapse;margin:0 0 7px 0;background:#F8FAFC;border:1px solid #E8EEF6;border-radius:9px;}"
           << ".history-row td{padding:10px 12px;vertical-align:middle;}"
           << ".guest-name{font-size:10pt;font-weight:700;color:#2B3674;margin-bottom:2px;}"
           << ".booking-meta{font-size:8.5pt;color:#8F9BB7;}"
           << ".stay-date{font-size:9pt;font-weight:600;color:#52637A;margin-top:5px;}"
           << ".room-chip{background:#EAF1FF;color:#2B6DEF;border-radius:8px;padding:3px 7px;font-size:8pt;font-weight:700;}"
           << ".pagination{width:100%;margin-top:12px;border-collapse:collapse;}"
           << ".page-link{display:inline-block;background:#FFFFFF;color:#52637A;border:1px solid #DCE5F0;border-radius:6px;padding:4px 8px;margin:0 3px;font-size:8.5pt;font-weight:700;text-decoration:none;}"
           << ".page-link-active{display:inline-block;background:#2B6DEF;color:#FFFFFF;border:1px solid #2B6DEF;border-radius:6px;padding:4px 8px;margin:0 3px;font-size:8.5pt;font-weight:700;text-decoration:none;}"
           << ".data-table{border-collapse:collapse;width:100%;margin-top:6px;}"
           << ".data-table th,.data-table td{border:1px solid #E8EEF6;padding:8px 9px;text-align:left;vertical-align:top;}"
           << ".data-table th{background:#F5F8FC;color:#52637A;font-size:8pt;font-weight:700;}"
           << ".data-table tr:nth-child(even){background:#FAFCFE;}"
           << ".warn{color:#B45309;font-weight:600;}"
           << "</style>";

    if (!m_manager) {
        stream << "<div class='audit-section'><div class='kicker'>COMPLETED STAYS</div>"
               << "<div class='section-title'>Booking history</div>"
               << "<div class='empty-state'><div class='state-mark'>&#8226;</div>"
               << "<div class='empty-title'>No data available</div>"
               << "<div class='note'>Connect the hotel manager to view completed bookings.</div></div>";
        stream << "</div>";
        return html;
    }

    struct HistoryRow {
        QString bookingId;
        QString customerName;
        QString roomNumber;
        QDate checkIn;
        QDate checkOut;
    };

    const QDate today = QDate::currentDate();
    const int rangeIndex = ui->cmbDateRange->currentIndex();
    const QString rangeLabel = ui->cmbDateRange->currentText();

    const auto isInSelectedRange = [today, rangeIndex](const QDate& date) {
        if (rangeIndex == 0) {
            return date == today;
        }
        if (rangeIndex == 1) {
            int dateYear = 0;
            int todayYear = 0;
            return date.weekNumber(&dateYear) == today.weekNumber(&todayYear) && dateYear == todayYear;
        }
        if (rangeIndex == 2) {
            return date.month() == today.month() && date.year() == today.year();
        }
        return date.year() == today.year();
    };

    std::vector<HistoryRow> historyRows;
    for (const auto& booking : m_manager->getBookings()) {
        if (!booking || booking->isDeleted()
            || m_manager->getBookingState(*booking) != BookingState::COMPLETED) {
            continue;
        }

        const QDate checkIn = QDate::fromString(QString::fromStdString(booking->getCheckInDate()), Qt::ISODate);
        const QDate checkOut = QDate::fromString(QString::fromStdString(booking->getCheckOutDate()), Qt::ISODate);
        if (!checkIn.isValid() || !checkOut.isValid() || checkOut > today || !isInSelectedRange(checkOut)) {
            continue;
        }

        const auto customer = booking->getCustomer();
        const auto room = booking->getRoom();
        historyRows.push_back({
            QString::fromStdString(booking->getBookingId()),
            customer ? QString::fromStdString(customer->getName()) : QString("Guest not available"),
            room ? QString::fromStdString(room->getRoomNumber()) : QString("—"),
            checkIn,
            checkOut
        });
    }

    std::sort(historyRows.begin(), historyRows.end(), [](const HistoryRow& a, const HistoryRow& b) {
        if (a.checkOut != b.checkOut) {
            return a.checkOut > b.checkOut;
        }
        return a.customerName < b.customerName;
    });

    // Modified and optimized performance: render only seven completed stays per page to keep history responsive and compact.
    constexpr int pageSize = 7;
    const int pageCount = qMax(1, static_cast<int>((historyRows.size() + pageSize - 1) / pageSize));
    const int currentPage = qBound(0, m_historyPage, pageCount - 1);
    const int firstRow = currentPage * pageSize;
    const int lastRow = qMin(firstRow + pageSize, static_cast<int>(historyRows.size()));

    stream << "<div class='audit-section'><table class='section-header' width='100%' cellspacing='0' cellpadding='0'><tr><td>"
           << "<div class='kicker'>COMPLETED STAYS</div>"
           << "<div class='section-title'>Booking history</div>"
           << "<div class='section-description'>Completed stays that checked out during " << escapeHtml(rangeLabel) << ".</div>"
           << "</td><td align='right' valign='middle'><div class='history-count'>" << historyRows.size() << " stay" << (historyRows.size() == 1 ? "" : "s") << "</div></td></tr></table>";

    if (historyRows.empty()) {
        stream << "<div class='empty-state'><div class='state-mark'>&#10003;</div>"
               << "<div class='empty-title'>No completed stays yet</div>"
               << "<div class='note'>Completed bookings in " << escapeHtml(rangeLabel) << " will appear here.</div></div>";
    } else {
        for (int i = firstRow; i < lastRow; ++i) {
            const auto& row = historyRows[i];
            stream << "<table class='history-row' cellspacing='0' cellpadding='0'><tr><td width='58%'>"
                   << "<div class='guest-name'>" << escapeHtml(row.customerName) << "</div>"
                   << "<div class='booking-meta'>Booking #" << escapeHtml(row.bookingId) << "</div>"
                   << "</td><td align='right'><span class='room-chip'>Room " << escapeHtml(row.roomNumber) << "</span>"
                   << "<div class='stay-date'>" << escapeHtml(row.checkIn.toString("dd MMM")) << " — "
                   << escapeHtml(row.checkOut.toString("dd MMM yyyy")) << "</div></td></tr></table>";
        }

        if (pageCount > 1) {
            stream << "<table class='pagination' width='100%' cellspacing='0' cellpadding='0'><tr><td align='center'>";
            for (int page = 0; page < pageCount; ++page) {
                const QString cssClass = page == currentPage ? "page-link-active" : "page-link";
                stream << "<a class='" << cssClass << "' href='page:" << page << "'>" << (page + 1) << "</a>";
            }
            stream << "</td></tr></table>";
        }
    }
    stream << "</div>";

    return html;
}

QString DashboardWidget::buildReportHtml() const
{
    // Modified and optimized performance: delegate report aggregation to ReportService so this widget only renders the PDF document.
    const DashboardReportData report = ReportService(m_manager).buildDashboardReport(
        ui->cmbDateRange->currentIndex(),
        ui->cmbDateRange->currentText());
    const QDateTime now = report.generatedAt;
    const QString& rangeLabel = report.rangeLabel;
    const QString& rangeName = report.rangeName;
    const int totalRooms = report.totalRooms;
    const int standardCount = report.standardRooms;
    const int deluxeCount = report.deluxeRooms;
    const int suiteCount = report.suiteRooms;
    const int occupiedRooms = report.occupiedRooms;
    const int upcomingCount = report.upcomingBookings;
    const int activeCount = report.activeBookings;
    const int completedCount = report.completedBookings;
    const int cancelledCount = report.cancelledBookingsCount;
    const int bookingsThisMonth = report.bookingsThisMonth;
    const int bookingsThisYear = report.bookingsThisYear;
    const double occupancyRate = report.occupancyRate;
    const auto& popularRooms = report.topRooms;
    const auto& scheduledBookings = report.openBookings;
    const auto& completedStays = report.completedStays;
    const auto& cancelledBookings = report.cancelledBookings;

    QString html;
    QTextStream stream(&html);
    // Modified and optimized performance: define print-safe wrapper classes so compact report sections remain cohesive across PDF page boundaries.
    stream << "<html><head><meta charset='utf-8'>"
           << "<style>"
            << "body{font-family:'Segoe UI',Arial,sans-serif;color:#24324A;font-size:9pt;line-height:1.42;margin:0;padding:0;}"
            << "h1{color:#142A5E;margin:0;font-size:24pt;font-weight:800;letter-spacing:.2px;}"
            << "h2{color:#142A5E;margin:0 0 7px;font-size:14pt;font-weight:750;}"
            << "table{border-collapse:collapse;width:100%;}"
            << ".header{background:#F4F7FE;border:1px solid #DCE6F5;border-radius:12px;padding:18px 20px;}"
            << ".eyebrow{color:#2B6DEF;font-size:8pt;font-weight:800;letter-spacing:1.2px;margin-bottom:4px;}"
            << ".meta{color:#6F819D;font-size:8.5pt;margin-top:7px;}"
            << ".range-badge{background:#E7F0FF;color:#1F5FD6;border:1px solid #CFE0FF;border-radius:8px;padding:6px 10px;font-size:8.5pt;font-weight:700;}"
            << ".section-note{color:#7B8BA5;font-size:8.5pt;margin:0 0 10px;}"
            << ".report-section{margin-top:22px;}"
            << ".report-section.compact{page-break-inside:avoid;}"
            << ".section-heading{page-break-after:avoid;}"
            << ".overview-grid,.compact-shell{margin-top:22px;border-collapse:separate;border-spacing:9px 0;page-break-inside:avoid;}"
            << ".detail-shell{margin-top:22px;page-break-inside:avoid;}"
            << ".page-break-before{page-break-before:always;}"
            << ".overview-grid td{width:50%;vertical-align:top;}"
            << ".small-card{border:1px solid #DFE8F5;border-radius:10px;padding:12px;background:#FFFFFF;}"
            << ".small-card h2{margin-top:0;}"
            << ".summary{border-collapse:separate;border-spacing:8px 0;margin-top:14px;}"
            << ".summary td{border:1px solid #DFE8F5;border-radius:10px;padding:12px 14px;background:#FFFFFF;width:20%;}"
            << ".metric-label{font-size:8pt;color:#7B8BA5;font-weight:650;}"
            << ".metric-value{font-size:19pt;font-weight:800;color:#1B3F83;margin-top:3px;}"
            << ".data-table{border:1px solid #DDE6F2;margin-top:8px;}"
            << ".data-table th{background:#EEF4FF;color:#25477D;font-size:8pt;font-weight:800;padding:8px 9px;text-align:left;border:1px solid #DDE6F2;}"
            << ".data-table td{padding:8px 9px;border:1px solid #E5ECF5;vertical-align:top;font-size:8.3pt;}"
            << ".data-table tr:nth-child(even) td{background:#FAFCFF;}"
            << ".status{font-weight:750;color:#2A5FC5;}"
            << ".empty{border:1px dashed #C9D6E8;color:#7B8BA5;background:#FBFCFE;padding:13px;text-align:center;font-size:8.5pt;}"
            << ".footer{margin-top:22px;padding-top:9px;border-top:1px solid #E2EAF4;color:#8A99B0;font-size:7.5pt;}"
            << ".compact .data-table{page-break-inside:avoid;}"
            << "tr{page-break-inside:avoid;}"
           << "</style></head><body>";

    // Modified and optimized performance: render a print-first report with full operational booking data instead of embedding the interactive dashboard card.
    stream << "<div class='header'><table><tr><td>"
           << "<div class='eyebrow'>HOTEL OPERATIONS REPORT</div>"
           << "<h1>Booking Management Dashboard</h1>"
           << "<div class='meta'>Generated " << escapeHtml(now.toString("dd MMM yyyy, HH:mm"))
           << " &nbsp;•&nbsp; Reporting period: " << escapeHtml(rangeLabel) << "</div>"
           << "</td><td align='right' valign='middle'><span class='range-badge'>" << escapeHtml(rangeName) << "</span></td></tr></table></div>";

    stream << "<table class='summary'><tr>";
    stream << "<td><div class='metric-label'>Total rooms</div><div class='metric-value'>" << totalRooms << "</div></td>";
    stream << "<td><div class='metric-label'>Occupied rooms</div><div class='metric-value'>" << occupiedRooms << "</div></td>";
    stream << "<td><div class='metric-label'>Occupancy rate</div><div class='metric-value'>" << qRound(occupancyRate) << "%</div></td>";
    stream << "<td><div class='metric-label'>Active bookings</div><div class='metric-value'>" << activeCount << "</div></td>";
    stream << "<td><div class='metric-label'>Completed stays</div><div class='metric-value'>" << completedCount << "</div></td>";
    stream << "</tr></table>";

    // Modified and optimized performance: use one-row wrapper tables for compact report blocks so Qt keeps their title and content on the same PDF page.
    stream << "<table class='overview-grid'><tr><td><div class='small-card'><h2>Portfolio overview</h2><div class='section-note'>Current operational status and booking volume.</div>";
    stream << "<table class='data-table'><tr><th>Upcoming</th><th>Active</th><th>Completed</th><th>Cancelled</th><th>Check-ins this month</th><th>Check-ins this year</th></tr>";
    stream << "<tr><td>" << upcomingCount << "</td><td>" << activeCount << "</td><td>" << completedCount << "</td><td>" << cancelledCount << "</td><td>" << bookingsThisMonth << "</td><td>" << bookingsThisYear << "</td></tr></table></div></td><td><div class='small-card'><h2>Room inventory</h2><div class='section-note'>Active room portfolio by category.</div>";

    stream << "<table class='data-table'><tr><th>Standard</th><th>Deluxe</th><th>Suite</th></tr>";
    stream << "<tr><td>" << standardCount << "</td><td>" << deluxeCount << "</td><td>" << suiteCount << "</td></tr></table></div></td></tr></table>";

    // Modified and optimized performance: begin the top-room summary on a fresh page so its heading and table always remain one cohesive PDF block.
    stream << "<table class='compact-shell page-break-before'><tr><td><div class='small-card'><h2>Top rooms in selected period</h2><div class='section-note'>Ranked by check-ins during " << escapeHtml(rangeLabel) << ".</div>";
    stream << "<table class='data-table'><tr><th width='8%'>#</th><th width='28%'>Room</th><th width='36%'>Type</th><th width='28%'>Bookings</th></tr>";
    const int topCount = std::min(3, static_cast<int>(popularRooms.size()));
    if (topCount == 0) {
        stream << "<tr><td colspan='4' class='empty'>No bookings in the selected period.</td></tr>";
    } else {
        for (int i = 0; i < topCount; ++i) {
            const auto& item = popularRooms[i];
            stream << "<tr><td>" << (i + 1) << "</td><td>" << escapeHtml(item.roomNumber) << "</td><td>" << escapeHtml(item.type) << "</td><td>" << item.bookingCount << "</td></tr>";
        }
    }
    stream << "</table></div></td></tr></table>";

    // Modified and optimized performance: keep each detailed report heading with its table, moving the complete block to the next page when needed.
    stream << "<table class='detail-shell'><tr><td><div class='small-card'><h2>Open booking activity</h2><div class='section-note'>Upcoming and active stays with check-in dates in the selected period.</div>";
    stream << "<table class='data-table'><tr><th>Booking ID</th><th>Guest</th><th>Customer ID</th><th>Phone</th><th>Room</th><th>Check-in</th><th>Planned check-out</th><th>Status</th></tr>";
    if (scheduledBookings.empty()) {
        stream << "<tr><td colspan='8' class='empty'>No open bookings in the selected period.</td></tr>";
    } else {
        for (const auto& entry : scheduledBookings) {
            stream << "<tr><td>" << escapeHtml(entry.bookingId) << "</td><td>" << escapeHtml(entry.customerName)
                   << "</td><td>" << escapeHtml(entry.customerId) << "</td><td>" << escapeHtml(entry.phone)
                   << "</td><td>" << escapeHtml(entry.roomNumber) << "<br/><span class='section-note'>" << escapeHtml(entry.roomType)
                   << "</span></td><td>" << escapeHtml(entry.checkIn.toString("dd MMM yyyy"))
                   << "</td><td>" << escapeHtml(entry.checkOut.toString("dd MMM yyyy"))
                   << "</td><td class='status'>" << escapeHtml(entry.status) << "</td></tr>";
        }
    }
    stream << "</table></div></td></tr></table>";

    // Modified and optimized performance: keep the cancellation heading and its empty state or table in one compact PDF block.
    stream << "<table class='compact-shell'><tr><td><div class='small-card'><h2>Cancelled reservations</h2><div class='section-note'>Reservations with check-in dates in the selected period.</div>";
    if (cancelledBookings.empty()) {
        stream << "<div class='empty'>No cancelled reservations in the selected period.</div>";
    } else {
        stream << "<table class='data-table'><tr><th>Booking ID</th><th>Guest</th><th>Customer ID</th><th>Room</th><th>Check-in</th><th>Planned check-out</th></tr>";
        for (const auto& entry : cancelledBookings) {
            stream << "<tr><td>" << escapeHtml(entry.bookingId) << "</td><td>" << escapeHtml(entry.customerName)
                   << "</td><td>" << escapeHtml(entry.customerId) << "</td><td>" << escapeHtml(entry.roomNumber)
                   << "<br/><span class='section-note'>" << escapeHtml(entry.roomType)
                   << "</span></td><td>" << escapeHtml(entry.checkIn.toString("dd MMM yyyy"))
                   << "</td><td>" << escapeHtml(entry.checkOut.toString("dd MMM yyyy")) << "</td></tr>";
        }
        stream << "</table>";
    }
    stream << "</div></td></tr></table>";

    // Modified and optimized performance: begin completed history on a new page so its complete heading, note, and table stay together.
    stream << "<table class='detail-shell page-break-before'><tr><td><div class='small-card'><h2>Completed booking history</h2><div class='section-note'>Bookings checked out during " << escapeHtml(rangeLabel) << ".</div>";
    stream << "<table class='data-table'><tr><th>Booking ID</th><th>Guest</th><th>Customer ID</th><th>Phone</th><th>Room</th><th>Check-in</th><th>Checked out</th></tr>";
    if (completedStays.empty()) {
        stream << "<tr><td colspan='7' class='empty'>No completed stays in the selected period.</td></tr>";
    } else {
        for (const auto& entry : completedStays) {
            stream << "<tr><td>" << escapeHtml(entry.bookingId) << "</td><td>" << escapeHtml(entry.customerName)
                   << "</td><td>" << escapeHtml(entry.customerId) << "</td><td>" << escapeHtml(entry.phone)
                   << "</td><td>" << escapeHtml(entry.roomNumber) << "<br/><span class='section-note'>" << escapeHtml(entry.roomType)
                   << "</span></td><td>" << escapeHtml(entry.checkIn.toString("dd MMM yyyy"))
                   << "</td><td>" << escapeHtml(entry.checkOut.toString("dd MMM yyyy")) << "</td></tr>";
        }
    }
    stream << "</table></div></td></tr></table>";

    stream << "<div class='footer'>Hotel Booking Management • Internal operational report • Generated automatically</div>";
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
    writer.setTitle("Booking Management Dashboard Report");
    writer.setCreator("Hotel Booking Management");

    QTextDocument document;
    document.setDefaultFont(QFont("Segoe UI", 9));
    document.setDocumentMargin(0);
    document.setHtml(buildReportHtml());
    document.setPageSize(QSizeF(pageLayout.paintRectPoints().size()));

    document.print(&writer);

    if (QFileInfo(finalPath).exists() && QFileInfo(finalPath).size() > 0) {
        QMessageBox::information(this, "Export Report", "Dashboard report exported successfully as PDF.");
        return;
    }

    QMessageBox::critical(this, "Export Error", "Cannot write the PDF report file.");
}

DashboardWidget::~DashboardWidget()
{
    delete ui;
}
