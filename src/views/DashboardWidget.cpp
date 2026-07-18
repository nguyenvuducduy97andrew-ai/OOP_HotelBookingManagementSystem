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
#include <vector>
#include <map>
#include <algorithm>

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

    // ---- Phần mới: dựng nội dung dashboard ----
    populateData();
    buildTrendChart();
    buildBarChart();
    applyStyle();
}

void DashboardWidget::updateDateTime()
{
    QString currentTime = QDateTime::currentDateTime().toString("DD/MM/YYYY HH:MM:SS");
    ui->lblDate->setText("Update " + currentTime);
}

// =================================================================
void DashboardWidget::populateData()
{
    if (!m_manager) {
        ui->statCard1->setData("Total Room Number", "0", "0 room type", true);
        ui->statCard2->setData("Occupancy rate", "0%", "No data available", true);
        ui->statCard3->setData("This month reservation", "0", "No data available", true);
        ui->statCard4->setData("This year reservations", "0", "No data available", true);
        ui->miniCard1->setData("📅", QColor("#E9EFFF"), "Upcoming", "0");
        ui->miniCard2->setData("🛏", QColor("#E9EFFF"), "Active", "0");
        ui->miniCard3->setData("✔", QColor("#E9EFFF"), "Completed", "0");
        ui->miniCard4->setData("✖", QColor("#FDE8E6"), "Cancelled", "0");
        return;
    }

    // 1. StatCard 1: Tổng số phòng & số loại phòng
    int totalRooms = m_manager->getRooms().size();
    int standardCount = 0;
    int deluxeCount = 0;
    int suiteCount = 0;
    for (const auto& r : m_manager->getRooms()) {
        if (!r) continue;
        if (dynamic_cast<StandardRoom*>(r.get())) {
            standardCount++;
        } else if (dynamic_cast<DeluxeRoom*>(r.get())) {
            deluxeCount++;
        } else if (dynamic_cast<SuiteRoom*>(r.get())) {
            suiteCount++;
        }
    }
    int roomTypes = 0;
    if (standardCount > 0) roomTypes++;
    if (deluxeCount > 0) roomTypes++;
    if (suiteCount > 0) roomTypes++;
    ui->statCard1->setData("Total Room Number", QString::number(totalRooms), QString::number(roomTypes) + " room types", true);

    // 2. StatCard 2: Occupancy rate
    double occupiedRooms = m_manager->getRoomsByOccupancy(true).size();
    double occupancyRate = 0.0;
    if (totalRooms > 0) {
        occupancyRate = (occupiedRooms / totalRooms) * 100.0;
    }
    ui->statCard2->setData("Occupancy rate", QString::number(qRound(occupancyRate)) + "%", 
                           "Currently occupied " + QString::number(occupiedRooms) + "/" + QString::number(totalRooms) + " rooms", true);

    // 3. StatCard 3: Đặt phòng tháng này
    QDate today = QDate::currentDate();
    int bookingsThisMonth = 0;
    for (const auto& b : m_manager->getBookings()) {
        if (!b) continue;
        QDate checkIn = QDate::fromString(QString::fromStdString(b->getCheckInDate()), Qt::ISODate);
        if (checkIn.isValid() && checkIn.month() == today.month() && checkIn.year() == today.year()) {
            bookingsThisMonth++;
        }
    }
    ui->statCard3->setData("This month's reservations", QString::number(bookingsThisMonth), 
                           "Total reservations for " + QString::number(today.month()), true);

    // 4. StatCard 4: Đặt phòng năm nay
    int bookingsThisYear = 0;
    for (const auto& b : m_manager->getBookings()) {
        if (!b) continue;
        QDate checkIn = QDate::fromString(QString::fromStdString(b->getCheckInDate()), Qt::ISODate);
        if (checkIn.isValid() && checkIn.year() == today.year()) {
            bookingsThisYear++;
        }
    }
    ui->statCard4->setData("This year reservations", QString::number(bookingsThisYear), 
                           "Total reservations for " + QString::number(today.year()), true);

    // 5. MiniCards: Trạng thái phòng (upcoming, active, completed, cancelled)
    int upcomingCount = 0;
    int activeCount = 0;
    int completedCount = 0;
    int cancelledCount = 0;
    for (const auto& b : m_manager->getBookings()) {
        if (!b) continue;
        BookingState state = m_manager->getBookingState(*b);
        switch (state) {
        case BookingState::UPCOMING: upcomingCount++; break;
        case BookingState::ACTIVE: activeCount++; break;
        case BookingState::COMPLETED: completedCount++; break;
        case BookingState::CANCELLED: cancelledCount++; break;
        }
    }
    ui->miniCard1->setData("📅", QColor("#E9EFFF"), "Upcoming", QString::number(upcomingCount));
    ui->miniCard2->setData("🛏", QColor("#E9EFFF"), "Active", QString::number(activeCount));
    ui->miniCard3->setData("✔", QColor("#E9EFFF"), "Completed", QString::number(completedCount));
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
        if (!b || b->isCancelled()) continue;
        
        QDate checkIn = QDate::fromString(QString::fromStdString(b->getCheckInDate()), Qt::ISODate);
        if (!checkIn.isValid()) continue;
        
        bool match = false;
        if (rangeIndex == 0) { // Hôm nay
            match = (checkIn == today);
        } else if (rangeIndex == 1) { // Tuần này
            int checkInYear = 0;
            int checkInWeek = checkIn.weekNumber(&checkInYear);
            int todayYear = 0;
            int todayWeek = today.weekNumber(&todayYear);
            match = (checkInWeek == todayWeek && checkInYear == todayYear);
        } else if (rangeIndex == 2) { // Tháng này
            match = (checkIn.month() == today.month() && checkIn.year() == today.year());
        } else if (rangeIndex == 3) { // Năm nay
            match = (checkIn.year() == today.year());
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
        
        QString typeLabel = "Standard";
        if (dynamic_cast<StandardRoom*>(room.get())) {
            typeLabel = "Standard";
        } else if (dynamic_cast<DeluxeRoom*>(room.get())) {
            typeLabel = "Deluxe";
        } else if (dynamic_cast<SuiteRoom*>(room.get())) {
            typeLabel = "Suite";
        }
        
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
            if (!b) continue;
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
            if (!b) continue;
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
    // Chỉ style phần thân mới thêm — không đụng lblTitle (đã có stylesheet
    // riêng trong .ui) hay cmbDateRange (đã có stylesheet riêng trong .ui).
    setStyleSheet(R"(
        DashboardWidget {
            background-color: #FFFFFF;
        }
        QLabel#lblDate {
            font-size: 12px;
            color: #A3AED0;
        }
        StatCard, MiniCard, #ChartCard {
            background-color: #FFFFFF;
            border: 1px solid #E9EDF7;
            border-radius: 14px;
        }
        #CardTitle {
            font-size: 12px;
            color: #A3AED0;
        }
        #CardValue {
            font-size: 22px;
            font-weight: 700;
            color: #2B3674;
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
            font-size: 11px;
            color: #A3AED0;
        }
        #MiniCardValue {
            font-size: 15px;
            font-weight: 700;
            color: #2B3674;
        }
        #ChartTitle {
            font-size: 13px;
            font-weight: 700;
            color: #2B3674;
            padding-bottom: 4px;
        }
        #RoomListItem {
            background-color: #F4F7FE;
            border-radius: 10px;
        }
        #RoomTitle {
            font-size: 13px;
            font-weight: 700;
            color: #2B3674;
        }
        #RoomSubtitle {
            font-size: 11px;
            color: #A3AED0;
        }
    )");
}

void DashboardWidget::refreshDashboard() {
    populateData();
    buildTrendChart();
    buildBarChart();
}

DashboardWidget::~DashboardWidget()
{
    delete ui;
}
