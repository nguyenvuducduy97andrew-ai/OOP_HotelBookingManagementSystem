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
#include <QTableWidget>
#include <QHeaderView>
#include <QTextStream>
#include <QFont>
#include <QStringList>
#include <QPageSize>
#include <QPageLayout>
#include <QFileInfo>
#include <QPainter>
#include <QScrollBar>
#include <QUrl>
#include <QLocale>
#include <vector>
#include <map>
#include <unordered_set>
#include <algorithm>
#include <cmath>

namespace {
const QString kReportSectionMarker = QStringLiteral("<!-- REPORT-SECTION -->");

QString escapeHtml(const QString& text)
{
    return text.toHtmlEscaped();
}

QString escapeHtmlNoWrap(const QString& text)
{
    return escapeHtml(text).replace(QLatin1Char(' '), QStringLiteral("&nbsp;"));
}

// Modified: Format dashboard report amounts locally with comma thousands separators for readable VND values.
QString formatMoney(double value)
{
    return QLocale(QLocale::English, QLocale::UnitedStates).toString(value, 'f', 0);
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

    // 1. Initialize the timer.
    dateTimeTimer = new QTimer(this);

    // 2. Connect the signal to a member function for a maintainable update flow.
    connect(dateTimeTimer, &QTimer::timeout, this, &DashboardWidget::updateDateTime);

    // 3. Start the timer.
    dateTimeTimer->start(1000);

    // Run once immediately so data is visible without waiting for the first tick.
    updateDateTime();

    // 4. Connect the time-range filter combo box.
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

    m_reservationPanel = new QFrame(ui->scrollAreaWidgetContents);
    m_reservationPanel->setObjectName("miniCardReservationPanel");
    auto *reservationLayout = new QVBoxLayout(m_reservationPanel);
    reservationLayout->setContentsMargins(14, 12, 14, 14);
    reservationLayout->setSpacing(5);
    m_reservationPanelTitle = new QLabel(m_reservationPanel);
    m_reservationPanelTitle->setObjectName("miniCardReservationTitle");
    m_reservationPanelSubtitle = new QLabel(m_reservationPanel);
    m_reservationPanelSubtitle->setObjectName("miniCardReservationSubtitle");
    m_reservationTable = new QTableWidget(m_reservationPanel);
    m_reservationTable->setObjectName("miniCardReservationTable");
    m_reservationTable->setColumnCount(7);
    m_reservationTable->setHorizontalHeaderLabels({
        "Booking ID", "Guest", "Room", "Planned check-in", "Planned check-out", "Status", "Actions"
    });
    m_reservationTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_reservationTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_reservationTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_reservationTable->verticalHeader()->hide();
    m_reservationTable->verticalHeader()->setDefaultSectionSize(44);
    m_reservationTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_reservationTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_reservationTable->setMaximumHeight(250);
    m_reservationTable->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    reservationLayout->addWidget(m_reservationPanelTitle);
    reservationLayout->addWidget(m_reservationPanelSubtitle);
    reservationLayout->addWidget(m_reservationTable);
    const int miniRowIndex = ui->bodyLayout ? ui->bodyLayout->indexOf(ui->miniRowWidget) : -1;
    if (ui->bodyLayout) {
        ui->bodyLayout->insertWidget(miniRowIndex + 1, m_reservationPanel);
    }
    m_reservationPanel->hide();

    connect(ui->miniCard1, &MiniCard::clicked, this, [this]() { toggleMiniCardReservations(1); });
    connect(ui->miniCard2, &MiniCard::clicked, this, [this]() { toggleMiniCardReservations(2); });
    connect(ui->miniCard3, &MiniCard::clicked, this, [this]() { toggleMiniCardReservations(3); });
    connect(ui->miniCard4, &MiniCard::clicked, this, [this]() { toggleMiniCardReservations(4); });

    // ---- Build dashboard content ----
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

void DashboardWidget::toggleMiniCardReservations(int cardIndex)
{
    if (m_selectedMiniCard == cardIndex && m_reservationPanel->isVisible()) {
        m_selectedMiniCard = 0;
        m_reservationPanel->hide();
        return;
    }
    m_selectedMiniCard = cardIndex;
    refreshMiniCardReservations();
    m_reservationPanel->show();
}

void DashboardWidget::refreshMiniCardReservations()
{
    if (!m_reservationTable || m_selectedMiniCard == 0) return;

    static const QString titles[] = {
        QString(), "Today's upcoming check-ins", "Currently staying",
        "Today's planned check-outs", "Cancelled reservations"
    };
    m_reservationPanelTitle->setText(titles[m_selectedMiniCard]);
    m_reservationTable->clearSpans();
    m_reservationTable->setRowCount(0);
    if (!m_manager) {
        m_reservationPanelSubtitle->setText("0 reservations");
        return;
    }
    const QDate today = QDate::currentDate();
    int row = 0;

    for (const auto& booking : m_manager->getBookings()) {
        if (!booking || booking->isDeleted()) continue;
        const BookingState state = m_manager->getBookingState(*booking);
        QDateTime plannedIn = QDateTime::fromString(QString::fromStdString(booking->getPlannedCheckInAt()), Qt::ISODateWithMs);
        QDateTime plannedOut = QDateTime::fromString(QString::fromStdString(booking->getPlannedCheckOutAt()), Qt::ISODateWithMs);
        if (!plannedIn.isValid()) plannedIn = QDateTime(QDate::fromString(QString::fromStdString(booking->getCheckInDate()), Qt::ISODate), QTime(0, 0));
        if (!plannedOut.isValid()) plannedOut = QDateTime(QDate::fromString(QString::fromStdString(booking->getCheckOutDate()), Qt::ISODate), QTime(0, 0));

        const bool matches =
            (m_selectedMiniCard == 1 && state == BookingState::UPCOMING && plannedIn.date() == today)
            || (m_selectedMiniCard == 2 && state == BookingState::ACTIVE)
            || (m_selectedMiniCard == 3 && (state == BookingState::UPCOMING || state == BookingState::ACTIVE) && plannedOut.date() == today)
            || (m_selectedMiniCard == 4 && (state == BookingState::CANCELLED || state == BookingState::NO_SHOW));
        if (!matches) continue;

        const auto customer = booking->getCustomer();
        const auto room = booking->getRoom();
        QString status;
        if (state == BookingState::UPCOMING) status = "Upcoming";
        else if (state == BookingState::ACTIVE) status = "Active";
        else status = "Cancelled";
        m_reservationTable->insertRow(row);
        m_reservationTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(booking->getBookingId())));
        m_reservationTable->setItem(row, 1, new QTableWidgetItem(customer ? QString::fromStdString(customer->getName()) : "Unavailable guest"));
        m_reservationTable->setItem(row, 2, new QTableWidgetItem(room ? QString::fromStdString(room->getRoomNumber()) : "—"));
        m_reservationTable->setItem(row, 3, new QTableWidgetItem(plannedIn.isValid() ? plannedIn.toString("dd MMM yyyy, HH:mm") : "—"));
        m_reservationTable->setItem(row, 4, new QTableWidgetItem(plannedOut.isValid() ? plannedOut.toString("dd MMM yyyy, HH:mm") : "—"));
        m_reservationTable->setItem(row, 5, new QTableWidgetItem(status));

        auto *actions = new QWidget(m_reservationTable);
        actions->setStyleSheet("background:transparent;");
        auto *actionLayout = new QHBoxLayout(actions);
        actionLayout->setContentsMargins(4, 2, 4, 2);
        actionLayout->setSpacing(6);
        const auto addAction = [this, actions, actionLayout, booking](const QString& text,
                                                                     const QString& actionType,
                                                                     const QString& colors) {
            auto *button = new QPushButton(text, actions);
            button->setCursor(Qt::PointingHandCursor);
            button->setMinimumHeight(26);
            button->setMinimumWidth(text == "Check out" ? 89 : 76);
            button->setStyleSheet("QPushButton { " + colors
                + " border-radius:7px; padding:0 9px; font-size:12px; font-weight:700; }"
                  "QPushButton:hover { color:#1B2559; border-color:#60A5FA; }");
            const QString bookingId = QString::fromStdString(booking->getBookingId());
            connect(button, &QPushButton::clicked, this, [this, bookingId, actionType]() {
                emit reservationActionRequested(bookingId, actionType);
            });
            actionLayout->addWidget(button);
        };
        if (state == BookingState::UPCOMING) {
            addAction("Check in", "checkin", "background: #EFF6FF; color:#1D4ED8; border:1px solid #BFDBFE;");
            addAction("Cancel", "cancel", "background: #FFFBEB; color:#92400E; border:1px solid #FDE68A;");
        } else if (state == BookingState::ACTIVE) {
            addAction("Check out", "checkout", "background: #ECFDF5; color:#065F46; border:1px solid #A7F3D0;");
        } else {
            auto *none = new QLabel("No available actions", actions);
            none->setStyleSheet("color: #94A3B8; background:transparent;");
            actionLayout->addWidget(none);
        }
        actionLayout->addStretch();
        m_reservationTable->setCellWidget(row, 6, actions);
        ++row;
    }

    m_reservationPanelSubtitle->setText(QString("%1 reservation%2").arg(row).arg(row == 1 ? "" : "s"));
    if (row == 0) {
        m_reservationTable->setRowCount(1);
        auto *empty = new QTableWidgetItem("No matching reservations.");
        empty->setTextAlignment(Qt::AlignCenter);
        m_reservationTable->setItem(0, 0, empty);
        m_reservationTable->setSpan(0, 0, 1, m_reservationTable->columnCount());
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
        ui->statCard1->setData("Total Room Number", "0", "0 room types", true);
        ui->statCard2->setData("Occupancy rate", "0%", "No data available", true);
        ui->statCard3->setData("This month's arrivals", "0", "No data available", true);
        ui->statCard4->setData("This year's arrivals", "0", "No data available", true);
        ui->miniCard1->setData("📅", QColor("#E8F0FF"), "Today's upcoming check-ins", "0");
        ui->miniCard2->setData("🛏", QColor("#E6FAF4"), "Currently staying", "0");
        ui->miniCard3->setData("✔", QColor("#F0EBFF"), "Today's planned check-outs", "0");
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
    int plannedCheckoutCount = 0;
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
        QDateTime plannedIn = QDateTime::fromString(QString::fromStdString(booking->getPlannedCheckInAt()), Qt::ISODateWithMs);
        QDateTime plannedOut = QDateTime::fromString(QString::fromStdString(booking->getPlannedCheckOutAt()), Qt::ISODateWithMs);
        if (!plannedIn.isValid()) plannedIn = QDateTime(QDate::fromString(QString::fromStdString(booking->getCheckInDate()), Qt::ISODate), QTime(0, 0));
        if (!plannedOut.isValid()) plannedOut = QDateTime(QDate::fromString(QString::fromStdString(booking->getCheckOutDate()), Qt::ISODate), QTime(0, 0));
        switch (state) {
        case BookingState::UPCOMING:
            if (plannedIn.date() == statsToday) upcomingCount++;
            if (plannedOut.date() == statsToday) plannedCheckoutCount++;
            break;
        case BookingState::ACTIVE:
            activeCount++;
            if (plannedOut.date() == statsToday) plannedCheckoutCount++;
            break;
        case BookingState::COMPLETED: break;
        case BookingState::CANCELLED: cancelledCount++; break;
        case BookingState::NO_SHOW: cancelledCount++; break;
        }

        // Modified: Dashboard arrival counters use staff-recorded actual check-in time, not the planned reservation date.
        if (state != BookingState::ACTIVE && state != BookingState::COMPLETED) {
            continue;
        }
        QDateTime actualCheckIn = QDateTime::fromString(
            QString::fromStdString(booking->getActualCheckInAt()), Qt::ISODateWithMs);
        if (!actualCheckIn.isValid()) {
            actualCheckIn = QDateTime(
                QDate::fromString(QString::fromStdString(booking->getActualCheckInDate()), Qt::ISODate), QTime(0, 0));
        }
        const QDate checkIn = actualCheckIn.date();
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
    // Modified: Label dashboard arrival KPIs as actual arrivals so staff do not confuse them with planned reservations.
    ui->statCard3->setData("This month's arrivals", QString::number(bookingsThisMonth),
                           "Actual check-ins for " + QString::number(statsToday.month()), true); 
    ui->statCard4->setData("This year's arrivals", QString::number(bookingsThisYear),
                           "Actual check-ins for " + QString::number(statsToday.year()), true);
    ui->miniCard1->setData("📅", QColor("#E8F0FF"), "Today's upcoming check-ins", QString::number(upcomingCount));
    ui->miniCard2->setData("🛏", QColor("#E6FAF4"), "Currently staying", QString::number(activeCount));
    ui->miniCard3->setData("✔", QColor("#F0EBFF"), "Today's planned check-outs", QString::number(plannedCheckoutCount));
    // Modified: Present one cancelled lifecycle bucket; a guest not arriving is captured in its cancellation reason.
    ui->miniCard4->setData("✖", QColor("#FDE8E6"), "Cancelled", QString::number(cancelledCount));

    // ---- Featured room list ----
    // 1. Update the title for the selected time range.
    int rangeIndex = ui->cmbDateRange->currentIndex();
    QString rangeText = "this month";
    if (rangeIndex == 0) rangeText = "today";
    else if (rangeIndex == 1) rangeText = "this week";
    else if (rangeIndex == 2) rangeText = "this month";
    else if (rangeIndex == 3) rangeText = "this year";

    ui->roomListTitleLabel->setText("Popular Rooms - " + rangeText);

    // 2. Calculate the booking count for each room in the selected period.
    std::map<std::string, int> roomBookingCounts;
    for (const auto& r : m_manager->getRooms()) {
        if (!r || r->isArchived()) continue;
        roomBookingCounts[r->getRoomNumber()] = 0;
    }

    for (const auto& b : m_manager->getBookings()) {
        if (!b || b->isDeleted()) continue;
        const BookingState state = m_manager->getBookingState(*b);
        if (state != BookingState::ACTIVE && state != BookingState::COMPLETED) continue;

        // Modified: Rank dashboard rooms by actual arrivals so planned reservations do not inflate demand.
        QDateTime actualCheckIn = QDateTime::fromString(
            QString::fromStdString(b->getActualCheckInAt()), Qt::ISODateWithMs);
        if (!actualCheckIn.isValid()) {
            actualCheckIn = QDateTime(
                QDate::fromString(QString::fromStdString(b->getActualCheckInDate()), Qt::ISODate), QTime(0, 0));
        }
        QDate checkIn = actualCheckIn.date();
        if (!checkIn.isValid()) continue;
        
        bool match = false;
        if (rangeIndex == 0) { // Today
            match = (checkIn == statsToday);
        } else if (rangeIndex == 1) { // This week
            int checkInYear = 0;
            int checkInWeek = checkIn.weekNumber(&checkInYear);
            int todayYear = 0;
            int todayWeek = statsToday.weekNumber(&todayYear);
            match = (checkInWeek == todayWeek && checkInYear == todayYear);
        } else if (rangeIndex == 2) { // This month
            match = (checkIn.month() == statsToday.month() && checkIn.year() == statsToday.year());
        } else if (rangeIndex == 3) { // This year
            match = (checkIn.year() == statsToday.year());
        }
        
        if (match) {
            auto room = b->getRoom();
            if (room && !room->isArchived()) {
                roomBookingCounts[room->getRoomNumber()]++;
            }
        }
    }

    // 3. Sort rooms by booking count in descending order.
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
            return a.bookingCount > b.bookingCount; // Highest count first.
        }
        return a.room->getRoomNumber() < b.room->getRoomNumber(); // Use room number as the tie-breaker.
    });

    // 4. Display the top three featured rooms.
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
        QColor badgeColor = QColor("#05CD99"); // Default green.
        
        if (i == 0) {
            subtitle = "Most booked";
        } else if (i == 1) {
            if (numToShow == 2) {
                subtitle = "Least booked";
                badgeColor = QColor("#EE5D50"); // Red
            } else {
                subtitle = "Second most booked";
            }
        } else if (i == 2) {
            bool isLeast = (roomStatsList.size() == 3) || (count == roomStatsList.back().bookingCount);
            if (isLeast) {
                subtitle = "Least booked";
                badgeColor = QColor("#EE5D50"); // Red
            } else {
                subtitle = "Third most booked";
                badgeColor = QColor("#005BFE"); // Blue
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
            const BookingState state = m_manager->getBookingState(*b);
            if (state != BookingState::ACTIVE && state != BookingState::COMPLETED) continue;
            // Modified: Trend charts count actual guest arrivals rather than reservations that may be cancelled or never checked in.
            QDateTime actualCheckIn = QDateTime::fromString(
                QString::fromStdString(b->getActualCheckInAt()), Qt::ISODateWithMs);
            if (!actualCheckIn.isValid()) {
                actualCheckIn = QDateTime(
                    QDate::fromString(QString::fromStdString(b->getActualCheckInDate()), Qt::ISODate), QTime(0, 0));
            }
            QDate checkIn = actualCheckIn.date();
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
            const BookingState state = m_manager->getBookingState(*b);
            if (state != BookingState::ACTIVE && state != BookingState::COMPLETED) continue;
            auto room = b->getRoom();
            if (!room) continue;
            if (room->getRoomTypeName() == "Standard") {
                standardBookings++;
            } else if (room->getRoomTypeName() == "Deluxe") {
                deluxeBookings++;
            } else if (room->getRoomTypeName() == "Suite") {
                suiteBookings++;
            }
        }
    }

    // Modified: Show actual-arrival counts by room type, not every planned reservation in the database.
    auto *set = new QBarSet("Actual arrivals");
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
        #MiniCard:hover { background-color:#F8FAFF; border-color:#BFDBFE; }
        QFrame#miniCardReservationPanel {
            background-color:#F8FAFC; border:1px solid #E0E8F5; border-radius:14px;
        }
        QLabel#miniCardReservationTitle { color:#1B2559; font-size:14px; font-weight:800; }
        QLabel#miniCardReservationSubtitle { color:#7B8BA5; font-size:12px; }
        QTableWidget#miniCardReservationTable {
            background:#FFFFFF; border:1px solid #E7EDF6; border-radius:10px;
            gridline-color:#EEF2F7; color:#2B3674; font-size:12px;
            selection-background-color:#EAF2FF; selection-color:#2B3674;
        }
        QTableWidget#miniCardReservationTable::item { padding:7px 9px; }
        QTableWidget#miniCardReservationTable::item:hover { background:#F8FAFF; color:#2B3674; border:none; }
        QTableWidget#miniCardReservationTable::item:selected { background:#EAF2FF; color:#2B3674; border:none; }
        QTableWidget#miniCardReservationTable QHeaderView::section {
            background:#F8FAFC; color:#A3AED0; font-weight:700; border:none;
            border-bottom:2px solid #E9EDF7; padding:8px;
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
    if (m_selectedMiniCard != 0) refreshMiniCardReservations();
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

        const QDate checkIn = QDate::fromString(QString::fromStdString(
            booking->getActualCheckInDate().empty() ? booking->getCheckInDate() : booking->getActualCheckInDate()), Qt::ISODate);
        // Modified: Show factual departure dates in completed history after an early or late checkout.
        const QDate checkOut = QDate::fromString(QString::fromStdString(booking->getEffectiveCheckOutDate()), Qt::ISODate);
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
    const double periodOccupiedRoomHours = report.periodOccupiedRoomHours;
    const double periodCleaningRoomHours = report.periodCleaningRoomHours;
    const double periodMaintenanceRoomHours = report.periodMaintenanceRoomHours;
    const double periodSaleableRoomHours = report.periodSaleableRoomHours;
    const double occupancyRate = report.occupancyRate;
    const double periodOccupancyRate = report.periodOccupancyRate;
    const double invoicedRevenue = report.invoicedRevenue;
    const double averageBilledHourlyRate = report.averageBilledHourlyRate;
    const double revenuePerSaleableRoomHour = report.revenuePerSaleableRoomHour;
    const auto& popularRooms = report.topRooms;
    const auto& plannedArrivals = report.plannedArrivals;
    const auto& plannedDepartures = report.plannedDepartures;
    const auto& scheduledCleaning = report.scheduledCleaning;
    const auto& maintenanceWindows = report.maintenanceWindows;
    const auto& actualCheckIns = report.actualCheckIns;
    const auto& actualCheckOuts = report.actualCheckOuts;
    const auto& cancelledBookings = report.cancelledBookings;

    QString html;
    QTextStream stream(&html);
    // Modified: define compact, balanced report styles for predictable PDF tables and pagination.
    stream << "<html><head><meta charset='utf-8'>"
           << "<style>"
            << "body{font-family:'Segoe UI',Arial,sans-serif;color:#24324A;font-size:8.2pt;line-height:1.28;margin:0;padding:0;}"
            << "h1{color:#142A5E;margin:0;font-size:21pt;font-weight:800;letter-spacing:.15px;}"
            << "h2{color:#142A5E;margin:0 0 4px;font-size:12pt;font-weight:750;}"
            << "table{border-collapse:collapse;width:100%;}"
            << ".header{background:#F4F7FE;border:1px solid #DCE6F5;border-radius:12px;padding:15px 18px;}"
            << ".header-table{width:100%;table-layout:fixed;}"
            << ".header-title{width:76%;vertical-align:middle;}"
            << ".header-range{width:24%;text-align:right;vertical-align:middle;}"
            << ".eyebrow{color:#2B6DEF;font-size:8pt;font-weight:800;letter-spacing:1.2px;margin-bottom:4px;}"
            << ".meta{color:#6F819D;font-size:8pt;margin-top:6px;}"
            << ".range-badge{background:#E7F0FF;color:#1F5FD6;border:1px solid #CFE0FF;border-radius:9px;padding:8px 13px;font-size:12pt;font-weight:800;}"
            << ".section-note{color:#71839F;font-size:7.4pt;margin:0 0 7px;}"
            << ".report-section{margin-top:14px;}"
            << ".report-section.compact{page-break-inside:avoid;}"
            << ".section-heading{page-break-after:avoid;}"
            << ".overview-grid,.compact-shell,.detail-shell{margin-top:12px;border-collapse:separate;border-spacing:6px 0;}"
            << ".overview-grid td{width:50%;vertical-align:top;}"
            << ".small-card{border:1px solid #DFE8F5;border-radius:10px;padding:10px 11px;background:#FFFFFF;}"
            << ".small-card h2{margin-top:0;}"
            << ".section-band{border-radius:8px;padding:7px 9px;margin:0 0 9px;}"
            << ".section-band .band-kicker{font-size:6.9pt;font-weight:800;letter-spacing:1px;margin-bottom:2px;}"
            << ".section-band .band-title{font-size:10pt;font-weight:800;}"
            << ".section-band.planned{background:#EFF6FF;border:1px solid #BFDBFE;color:#1D4ED8;}"
            << ".section-band.actual{background:#ECFDF5;border:1px solid #A7F3D0;color:#047857;}"
            << ".summary{border-collapse:separate;border-spacing:6px 0;margin-top:11px;page-break-inside:avoid;break-inside:avoid;}"
            << ".summary td{border:1px solid #DFE8F5;border-radius:10px;padding:9px 10px;background:#FFFFFF;width:16.66%;}"
            << ".metric-label{font-size:7.3pt;color:#7B8BA5;font-weight:650;}"
            << ".metric-value{font-size:16pt;font-weight:800;color:#1B3F83;margin-top:2px;}"
            << ".data-table{border:1px solid #DDE6F2;margin-top:6px;table-layout:fixed;}"
            << ".data-table th{background:#EEF4FF;color:#25477D;font-size:7.2pt;font-weight:800;padding:6px 7px;text-align:left;border:1px solid #DDE6F2;white-space:nowrap;}"
            << ".data-table td{padding:6px 7px;border:1px solid #E5ECF5;vertical-align:top;font-size:7.35pt;white-space:nowrap;}"
            << ".data-table tr:nth-child(even) td{background:#FAFCFF;}"
            << ".status{font-weight:750;color:#2A5FC5;}"
            << ".reason-cell{color:#5F718F;font-size:7pt;white-space:normal!important;word-wrap:break-word;line-height:1.25;}"
            << ".empty{border:1px dashed #C9D6E8;color:#7B8BA5;background:#FBFCFE;padding:10px;text-align:center;font-size:7.5pt;}"
            << ".footer{margin-top:14px;padding-top:7px;border-top:1px solid #E2EAF4;color:#8A99B0;font-size:7pt;}"
            << ".compact .data-table{page-break-inside:avoid;}"
           << "</style></head><body>";

    // Modified: mark complete report sections so the exporter can move a full block to the next page instead of splitting it.
    stream << kReportSectionMarker;
    // Modified and optimized performance: render a print-first report with full operational booking data instead of embedding the interactive dashboard card.
    stream << "<div class='header'><table class='header-table'><tr><td class='header-title' width='76%'>"
           << "<div class='eyebrow'>HOTEL OPERATIONS REPORT</div>"
           << "<h1>Booking Management Dashboard</h1>"
           << "<div class='meta'>Generated " << escapeHtml(now.toString("dd MMM yyyy, HH:mm"))
           << " &nbsp;•&nbsp; Reporting period: " << escapeHtml(rangeLabel) << "</div>"
           << "</td><td class='header-range' width='24%' align='right' valign='middle'><span class='range-badge'>" << escapeHtml(rangeName) << "</span></td></tr></table></div>";

    stream << "<table class='summary'><tr>";
    stream << "<td><div class='metric-label'>Total rooms</div><div class='metric-value'>" << totalRooms << "</div></td>";
    stream << "<td><div class='metric-label'>Currently occupied rooms</div><div class='metric-value'>" << occupiedRooms << "</div></td>";
    stream << "<td><div class='metric-label'>Current occupancy</div><div class='metric-value'>" << qRound(occupancyRate) << "%</div></td>";
    // Modified: Separate the real-time snapshot from actual room-hour occupancy so time-based reports cannot imply nightly billing.
    stream << "<td><div class='metric-label'>Period occupancy (to date)</div><div class='metric-value'>" << qRound(periodOccupancyRate) << "%</div><div class='section-note'>"
           << QString::number(periodOccupiedRoomHours, 'f', 1) << " / " << QString::number(periodSaleableRoomHours, 'f', 1)
           << " saleable room-hours</div></td>";
    stream << "<td><div class='metric-label'>Active bookings</div><div class='metric-value'>" << activeCount << "</div></td>";
    stream << "<td><div class='metric-label'>Completed stays</div><div class='metric-value'>" << completedCount << "</div></td>";
    stream << "</tr></table>";

    stream << kReportSectionMarker;
    // Modified and optimized performance: use one-row wrapper tables for compact report blocks so Qt keeps their title and content on the same PDF page.
    stream << "<table class='overview-grid'><tr><td><div class='small-card'><h2>Portfolio overview</h2><div class='section-note'>Current operational status and booking volume.</div>";
    stream << "<table class='data-table'><tr><th>Upcoming</th><th>Active</th><th>Completed</th><th>Cancelled</th><th>Check-ins this month</th><th>Check-ins this year</th></tr>";
    stream << "<tr><td>" << upcomingCount << "</td><td>" << activeCount << "</td><td>" << completedCount << "</td><td>" << cancelledCount << "</td><td>" << bookingsThisMonth << "</td><td>" << bookingsThisYear << "</td></tr></table></div></td><td><div class='small-card'><h2>Room inventory</h2><div class='section-note'>Active room portfolio by category.</div>";

    stream << "<table class='data-table'><tr><th>Standard</th><th>Deluxe</th><th>Suite</th></tr>";
    stream << "<tr><td>" << standardCount << "</td><td>" << deluxeCount << "</td><td>" << suiteCount << "</td></tr></table></div></td></tr></table>";

    stream << kReportSectionMarker;
    // Modified: Keep the Top Rooms heading, note, and table as one block without forcing a new page when remaining space is sufficient.
    // Modified: Identify this dashboard summary as actual arrival demand rather than planned booking volume.
    stream << "<table class='compact-shell'><tr><td><div class='small-card'><h2>Room demand summary</h2><div class='section-note'>Ranked by actual check-ins during " << escapeHtml(rangeLabel) << ".</div>";
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

    stream << kReportSectionMarker;
    // Modified: Render planned worklists separately so scheduled work is never mistaken for actual use or financial performance.
    // Modified: Use a visible planned-operations band so scheduling work is visually distinct from actual room and financial results.
    // Modified: Keep individual report titles concise because the surrounding visual band already supplies their operational context.
    stream << "<table class='detail-shell'><tr><td><div class='small-card'><div class='section-band planned'><div class='band-kicker'>PLANNED OPERATIONS</div><div class='band-title'>Schedule and room preparation</div></div><h2>Planned arrivals</h2><div class='section-note'>Reservations scheduled to arrive during " << escapeHtml(rangeLabel) << ".</div>";
    stream << "<table class='data-table'><tr><th width='12%'>Booking&nbsp;ID</th><th width='22%'>Guest</th><th width='14%'>Customer&nbsp;ID</th><th width='11%'>Room</th><th width='22%'>Planned&nbsp;arrival</th><th width='19%'>Current&nbsp;status</th></tr>";
    if (plannedArrivals.empty()) {
        stream << "<tr><td colspan='6' class='empty'>No planned arrivals in the selected period.</td></tr>";
    } else {
        for (const auto& entry : plannedArrivals) {
            stream << "<tr><td>" << escapeHtml(entry.bookingId) << "</td><td>" << escapeHtmlNoWrap(entry.customerName)
                   << "</td><td>" << escapeHtml(entry.customerId) << "</td><td>" << escapeHtml(entry.roomNumber)
                   << "<br/><span class='section-note'>" << escapeHtml(entry.roomType)
                   << "</span></td><td>" << escapeHtml(entry.plannedCheckInAt.toString("dd MMM yyyy HH:mm"))
                   << "</td><td class='status'>" << escapeHtml(entry.status) << "</td></tr>";
        }
    }
    stream << "</table></div></td></tr></table>";

    stream << kReportSectionMarker;
    stream << "<table class='detail-shell'><tr><td><div class='small-card'><h2>Planned departures</h2><div class='section-note'>Reservations scheduled to depart during " << escapeHtml(rangeLabel) << ".</div>";
    stream << "<table class='data-table'><tr><th width='12%'>Booking&nbsp;ID</th><th width='22%'>Guest</th><th width='14%'>Customer&nbsp;ID</th><th width='11%'>Room</th><th width='22%'>Planned&nbsp;departure</th><th width='19%'>Current&nbsp;status</th></tr>";
    if (plannedDepartures.empty()) {
        stream << "<tr><td colspan='6' class='empty'>No planned departures in the selected period.</td></tr>";
    } else {
        for (const auto& entry : plannedDepartures) {
            stream << "<tr><td>" << escapeHtml(entry.bookingId) << "</td><td>" << escapeHtmlNoWrap(entry.customerName)
                   << "</td><td>" << escapeHtml(entry.customerId) << "</td><td>" << escapeHtml(entry.roomNumber)
                   << "<br/><span class='section-note'>" << escapeHtml(entry.roomType)
                   << "</span></td><td>" << escapeHtml(entry.plannedCheckOutAt.toString("dd MMM yyyy HH:mm"))
                   << "</td><td class='status'>" << escapeHtml(entry.status) << "</td></tr>";
        }
    }
    stream << "</table></div></td></tr></table>";

    const auto renderOperationalBlocks = [&stream](const std::vector<ReportOperationalBlockEntry>& blocks, const QString& emptyMessage) {
        if (blocks.empty()) {
            stream << "<tr><td colspan='5' class='empty'>" << escapeHtml(emptyMessage) << "</td></tr>";
            return;
        }
        for (const auto& block : blocks) {
            stream << "<tr><td>" << escapeHtml(block.roomNumber) << "<br/><span class='section-note'>" << escapeHtml(block.roomType)
                   << "</span></td><td>" << escapeHtml(block.startsAt.toString("dd MMM yyyy HH:mm"))
                   << "</td><td>" << escapeHtml(block.endsAt.toString("dd MMM yyyy HH:mm"))
                   << "</td><td class='status'>" << escapeHtml(block.status)
                   << "</td><td class='reason-cell'>" << (block.note.trimmed().isEmpty() ? QStringLiteral("—") : escapeHtml(block.note))
                   << "</td></tr>";
        }
    };

    stream << kReportSectionMarker;
    stream << "<table class='detail-shell'><tr><td><div class='small-card'><h2>Scheduled cleaning</h2><div class='section-note'>Cleaning blocks created after checkout or released early when the room is marked ready.</div>";
    stream << "<table class='data-table'><tr><th width='16%'>Room</th><th width='24%'>Starts</th><th width='24%'>Ends</th><th width='16%'>Status</th><th width='20%'>Note</th></tr>";
    renderOperationalBlocks(scheduledCleaning, "No scheduled cleaning blocks in the selected period.");
    stream << "</table></div></td></tr></table>";

    stream << kReportSectionMarker;
    stream << "<table class='detail-shell'><tr><td><div class='small-card'><h2>Maintenance windows</h2><div class='section-note'>Confirmed maintenance is physical downtime; awaiting guest response remains a planning hold.</div>";
    stream << "<table class='data-table'><tr><th width='16%'>Room</th><th width='24%'>Starts</th><th width='24%'>Ends</th><th width='16%'>Status</th><th width='20%'>Note</th></tr>";
    renderOperationalBlocks(maintenanceWindows, "No maintenance windows in the selected period.");
    stream << "</table></div></td></tr></table>";

    stream << kReportSectionMarker;
    // Modified: Start the actual-and-finance view only after all planned worklists, preventing planned schedules from being read as actual performance.
    // Modified: Give actual performance and finance a separate visual band so it cannot be confused with planned operations.
    stream << "<table class='compact-shell'><tr><td><div class='small-card'><div class='section-band actual'><div class='band-kicker'>ACTUAL OPERATIONS AND FINANCE</div><div class='band-title'>Recorded room use and issued invoices</div></div><h2>Actual operations and finance</h2><div class='section-note'>Physical room use and issued invoices recorded during the selected period to date.</div>";
    stream << "<table class='data-table'><tr><th>Occupied room-hours</th><th>Cleaning hours</th><th>Maintenance hours</th><th>Available / saleable hours</th></tr>";
    stream << "<tr><td>" << QString::number(periodOccupiedRoomHours, 'f', 1) << "</td><td>"
           << QString::number(periodCleaningRoomHours, 'f', 1) << "</td><td>"
           << QString::number(periodMaintenanceRoomHours, 'f', 1) << "</td><td>"
           << QString::number(periodSaleableRoomHours, 'f', 1) << "</td></tr></table></div></td></tr></table>";

    stream << kReportSectionMarker;
    stream << "<table class='compact-shell'><tr><td><div class='small-card'><h2>Invoiced revenue</h2><div class='section-note'>Based only on invoices issued for completed stays. Planned check-in and check-out times never create revenue.</div>";
    stream << "<table class='data-table'><tr><th>Invoiced revenue</th><th>Avg. billed hourly rate</th><th>Revenue / saleable hour</th></tr>";
    // Modified: render all PDF revenue values with comma-grouped VND formatting.
    stream << "<tr><td>" << formatMoney(invoicedRevenue) << " VND</td><td>"
           << formatMoney(averageBilledHourlyRate) << " VND</td><td>"
           << formatMoney(revenuePerSaleableRoomHour) << " VND</td></tr></table></div></td></tr></table>";

    stream << kReportSectionMarker;
    // Modified: List actual check-ins explicitly so shift staff can reconcile physical arrivals against the planned-arrivals worklist.
    stream << "<table class='detail-shell'><tr><td><div class='small-card'><h2>Actual check-ins</h2><div class='section-note'>Guests physically checked in during " << escapeHtml(rangeLabel) << ".</div>";
    stream << "<table class='data-table'><tr><th width='13%'>Booking&nbsp;ID</th><th width='24%'>Guest</th><th width='16%'>Customer&nbsp;ID</th><th width='12%'>Room</th><th width='23%'>Actual&nbsp;check-in</th><th width='12%'>Status</th></tr>";
    if (actualCheckIns.empty()) {
        stream << "<tr><td colspan='6' class='empty'>No actual check-ins in the selected period.</td></tr>";
    } else {
        for (const auto& entry : actualCheckIns) {
            stream << "<tr><td>" << escapeHtml(entry.bookingId) << "</td><td>" << escapeHtmlNoWrap(entry.customerName)
                   << "</td><td>" << escapeHtml(entry.customerId) << "</td><td>" << escapeHtml(entry.roomNumber)
                   << "<br/><span class='section-note'>" << escapeHtml(entry.roomType)
                   << "</span></td><td>" << escapeHtml(entry.actualCheckInAt.toString("dd MMM yyyy HH:mm"))
                   << "</td><td class='status'>" << escapeHtml(entry.status) << "</td></tr>";
        }
    }
    stream << "</table></div></td></tr></table>";

    stream << kReportSectionMarker;
    // Modified: Keep cancellation audit rows together; a guest who did not arrive is recorded as a cancellation reason, not a competing lifecycle state.
    stream << "<table class='compact-shell'><tr><td><div class='small-card'><h2>Cancelled reservations</h2><div class='section-note'>Reservations with planned check-in times in the selected period.</div>";
    if (cancelledBookings.empty()) {
        stream << "<div class='empty'>No cancelled reservations in the selected period.</div>";
    } else {
        stream << "<table class='data-table'><tr><th width='10%'>Booking&nbsp;ID</th><th width='16%'>Guest</th><th width='13%'>Customer&nbsp;ID</th><th width='8%'>Room</th><th width='13%'>Planned&nbsp;check-in</th><th width='15%'>Planned&nbsp;check-out</th><th width='9%'>Status</th><th width='16%'>Reason</th></tr>";
        const auto renderReleasedReservation = [&stream](const ReportBookingEntry& entry) {
            QString reason = entry.operationalReason.trimmed();
            stream << "<tr><td>" << escapeHtml(entry.bookingId) << "</td><td>" << escapeHtmlNoWrap(entry.customerName)
                   << "</td><td>" << escapeHtml(entry.customerId) << "</td><td>" << escapeHtml(entry.roomNumber)
                   << "<br/><span class='section-note'>" << escapeHtml(entry.roomType)
                   << "</span></td><td>" << escapeHtml(entry.plannedCheckInAt.toString("dd MMM yyyy HH:mm"))
                   << "</td><td>" << escapeHtml(entry.plannedCheckOutAt.toString("dd MMM yyyy HH:mm"))
                   << "</td><td class='status'>" << escapeHtml(entry.status)
                   << "</td><td class='reason-cell'>" << (reason.isEmpty() ? QStringLiteral("—") : escapeHtml(reason))
                   << "</td></tr>";
        };
        for (const auto& entry : cancelledBookings) {
            renderReleasedReservation(entry);
        }
        stream << "</table>";
    }
    stream << "</div></td></tr></table>";

    stream << kReportSectionMarker;
    // Modified: Report actual departures as completed-stay history, keeping this operational fact distinct from planned departures.
    stream << "<table class='detail-shell'><tr><td><div class='small-card'><h2>Actual check-outs</h2><div class='section-note'>Completed stays checked out during " << escapeHtml(rangeLabel) << ".</div>";
    stream << "<table class='data-table'><tr><th width='12%'>Booking&nbsp;ID</th><th width='20%'>Guest</th><th width='16%'>Customer&nbsp;ID</th><th width='15%'>Phone</th><th width='9%'>Room</th><th width='14%'>Actual&nbsp;check-in</th><th width='14%'>Actual&nbsp;check-out</th></tr>";
    if (actualCheckOuts.empty()) {
        stream << "<tr><td colspan='7' class='empty'>No completed stays in the selected period.</td></tr>";
    } else {
        for (const auto& entry : actualCheckOuts) {
            stream << "<tr><td>" << escapeHtml(entry.bookingId) << "</td><td>" << escapeHtmlNoWrap(entry.customerName)
                   << "</td><td>" << escapeHtml(entry.customerId) << "</td><td>" << escapeHtml(entry.phone)
                   << "</td><td>" << escapeHtml(entry.roomNumber) << "<br/><span class='section-note'>" << escapeHtml(entry.roomType)
                   << "</span></td><td>" << escapeHtml(entry.actualCheckInAt.toString("dd MMM yyyy HH:mm"))
                   << "</td><td>" << escapeHtml(entry.actualCheckOutAt.toString("dd MMM yyyy HH:mm")) << "</td></tr>";
        }
    }
    stream << "</table></div></td></tr></table>";

    stream << kReportSectionMarker;
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
    // Modified: use a predictable point-sized PDF canvas so complete HTML sections can be measured before pagination.
    writer.setResolution(72);
    writer.setTitle("Booking Management Dashboard Report");
    writer.setCreator("Hotel Booking Management");

    // Modified: paginate each logical report block manually because QTextDocument does not reliably honor CSS page-break avoidance.
    const QString reportHtml = buildReportHtml();
    const QString bodyStartToken = QStringLiteral("</style></head><body>");
    const int bodyStart = reportHtml.indexOf(bodyStartToken);
    const int bodyEnd = reportHtml.lastIndexOf(QStringLiteral("</body></html>"));
    if (bodyStart < 0 || bodyEnd < bodyStart) {
        QMessageBox::critical(this, "Export Error", "Cannot prepare the report layout for PDF export.");
        return;
    }

    const QString documentPreamble = reportHtml.left(bodyStart + bodyStartToken.size());
    const QString documentBody = reportHtml.mid(bodyStart + bodyStartToken.size(), bodyEnd - bodyStart - bodyStartToken.size());
    const QStringList reportSections = documentBody.split(kReportSectionMarker, Qt::SkipEmptyParts);
    const QRect printRect = pageLayout.paintRectPixels(writer.resolution());

    QPainter painter(&writer);
    qreal verticalOffset = 0;
    constexpr qreal sectionGap = 8;

    const auto startNewPage = [&writer, &verticalOffset]() {
        writer.newPage();
        verticalOffset = 0;
    };

    for (const QString& section : reportSections) {
        QTextDocument sectionDocument;
        sectionDocument.setDefaultFont(QFont("Segoe UI", 9));
        sectionDocument.setDocumentMargin(0);
        sectionDocument.setHtml(documentPreamble + section + QStringLiteral("</body></html>"));
        sectionDocument.setTextWidth(printRect.width());

        const qreal sectionHeight = std::ceil(sectionDocument.size().height());
        if (sectionHeight <= printRect.height()
            && verticalOffset > 0
            && verticalOffset + sectionHeight > printRect.height()) {
            startNewPage();
        }

        qreal renderedHeight = 0;
        while (renderedHeight < sectionHeight) {
            const qreal availableHeight = printRect.height() - verticalOffset;
            if (availableHeight <= 0) {
                startNewPage();
                continue;
            }

            const qreal sliceHeight = std::min(availableHeight, sectionHeight - renderedHeight);
            painter.save();
            painter.setClipRect(QRectF(printRect.left(), printRect.top() + verticalOffset,
                                      printRect.width(), sliceHeight));
            painter.translate(printRect.left(), printRect.top() + verticalOffset - renderedHeight);
            sectionDocument.drawContents(&painter);
            painter.restore();

            renderedHeight += sliceHeight;
            verticalOffset += sliceHeight;
            if (renderedHeight < sectionHeight) {
                startNewPage();
            }
        }

        if (verticalOffset + sectionGap <= printRect.height()) {
            verticalOffset += sectionGap;
        }
    }
    painter.end();

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
