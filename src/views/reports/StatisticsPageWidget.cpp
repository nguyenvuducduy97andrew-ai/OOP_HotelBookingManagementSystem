#include "StatisticsPageWidget.h"
#include "InvoiceDialog.h"
#include "StandardRoom.h"
#include "DeluxeRoom.h"
#include "SuiteRoom.h"
#include "SearchFieldUi.h"
#include "SchedulePickerDialog.h"
#include <QHeaderView>
#include <QDateTime>
#include <QMessageBox>
#include <QScrollArea>
#include <QEvent>
#include <QFontMetrics>
#include <QFrame>
#include <QGuiApplication>
#include <QScreen>
#include <QLocale>
#include <map>

namespace {
// Modified: Format statistics amounts locally with comma thousands separators for readable VND values.
QString formatMoney(double value)
{
    return QLocale(QLocale::English, QLocale::UnitedStates).toString(value, 'f', 0);
}

class InvoiceActionTooltipFilter : public QObject
{
public:
    InvoiceActionTooltipFilter(QWidget* anchor, QString tooltipText, QObject* parent = nullptr)
        : QObject(parent), m_anchor(anchor), m_tooltipText(std::move(tooltipText))
    {
        m_popup = new QFrame(nullptr, Qt::ToolTip | Qt::FramelessWindowHint);
        m_popup->setObjectName("invoiceActionTooltipPopup");
        m_popup->setStyleSheet(
            "QFrame#invoiceActionTooltipPopup {"
            " background-color: #FFFFFF;"
            " color: #000000;"
            " border: 1px solid #0679de;"
            " border-radius: 6px;"
            "}"
        );

        auto* layout = new QHBoxLayout(m_popup);
        layout->setContentsMargins(10, 6, 10, 6);

        auto* label = new QLabel(m_tooltipText, m_popup);
        label->setStyleSheet("color: #000000; font-size: 12px; font-weight: 600;");
        label->setWordWrap(false);
        layout->addWidget(label);

        const QFontMetrics metrics(label->font());
        m_popup->setFixedSize(metrics.horizontalAdvance(m_tooltipText) + 24, metrics.height() + 12);
    }

    ~InvoiceActionTooltipFilter() override
    {
        if (m_popup) {
            m_popup->hide();
            m_popup->deleteLater();
        }
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched != m_anchor) {
            return QObject::eventFilter(watched, event);
        }

        if (event->type() == QEvent::Enter) {
            showPopup();
        } else if (event->type() == QEvent::Leave || event->type() == QEvent::Hide) {
            if (m_popup) m_popup->hide();
        }

        return QObject::eventFilter(watched, event);
    }

private:
    void showPopup()
    {
        if (!m_anchor || !m_popup) return;

        const QScreen* screen = QGuiApplication::screenAt(m_anchor->mapToGlobal(QPoint(0, 0)));
        if (!screen) screen = QGuiApplication::primaryScreen();

        const QRect available = screen ? screen->availableGeometry() : QRect();
        QPoint target = m_anchor->mapToGlobal(
            QPoint((m_anchor->width() - m_popup->width()) / 2, m_anchor->height() + 6));

        if (target.y() + m_popup->height() > available.bottom()) {
            target.setY(m_anchor->mapToGlobal(QPoint(0, 0)).y() - m_popup->height() - 6);
        }
        if (available.isValid()) {
            const int minX = available.left() + 6;
            const int maxX = available.right() - m_popup->width() - 6;
            if (target.x() < minX) target.setX(minX);
            if (target.x() > maxX) target.setX(maxX);
            if (target.y() < available.top() + 6) target.setY(available.top() + 6);
        }

        m_popup->move(target);
        m_popup->show();
        m_popup->raise();
    }

    QWidget* m_anchor;
    QString m_tooltipText;
    QFrame* m_popup = nullptr;
};

}

StatisticsPageWidget::StatisticsPageWidget(HotelManager* manager, QWidget* parent)
    : QWidget(parent), m_manager(manager)
{
    setupUI();
    refreshData();
}

QLabel* StatisticsPageWidget::createSectionHeader(const QString& text, QWidget* parent) {
    QLabel* label = new QLabel(text, parent);
    label->setStyleSheet("font-size: 16px; font-weight: bold; color: #2B3674; margin-bottom: 5px;");
    return label;
}

void StatisticsPageWidget::setupTopChartsUI(QWidget* parent, QVBoxLayout* mainLayout) {
    auto* chartsLayout1 = new QHBoxLayout();
    chartsLayout1->setSpacing(20);

    auto* barLayout = new QVBoxLayout();
    barLayout->setContentsMargins(0, 0, 0, 0);
    // Modified: Name finance charts after issued invoices so the statistical view does not imply planned-booking revenue.
    barLayout->addWidget(createSectionHeader("📈 Monthly invoiced revenue", parent));
    auto* barChartView = new QChartView(m_barChart);
    barChartView->setRenderHint(QPainter::Antialiasing);
    barChartView->setStyleSheet("background-color: #FFFFFF; border-radius: 12px; border: 1px solid #E2E8F0;");
    barChartView->setMinimumHeight(350);
    barLayout->addWidget(barChartView, 1);

    auto* pieLayout = new QVBoxLayout();
    pieLayout->setContentsMargins(0, 0, 0, 0);
    // Modified: Use an invoice symbol for the revenue breakdown instead of depicting the chart's donut geometry.
    pieLayout->addWidget(createSectionHeader("🧾 Selected-period invoiced revenue structure", parent));
    auto* pieChartView = new QChartView(m_pieChart);
    pieChartView->setRenderHint(QPainter::Antialiasing);
    pieChartView->setStyleSheet("background-color: #FFFFFF; border-radius: 12px; border: 1px solid #E2E8F0;");
    pieChartView->setMinimumHeight(350);
    pieLayout->addWidget(pieChartView, 1);

    chartsLayout1->addLayout(barLayout, 6);
    chartsLayout1->addLayout(pieLayout, 4);
    mainLayout->addLayout(chartsLayout1);
}

void StatisticsPageWidget::setupBottomChartsUI(QWidget* parent, QVBoxLayout* mainLayout) {
    auto* chartsLayout2 = new QHBoxLayout();
    chartsLayout2->setSpacing(20);

    auto* dowLayout = new QVBoxLayout();
    dowLayout->setContentsMargins(0, 0, 0, 0);
    dowLayout->addWidget(createSectionHeader("📅 Weekly actual arrivals", parent));
    auto* dowChartView = new QChartView(m_dowChart);
    dowChartView->setRenderHint(QPainter::Antialiasing);
    dowChartView->setStyleSheet("background-color: #FFFFFF; border-radius: 12px; border: 1px solid #E2E8F0;");
    dowChartView->setMinimumHeight(350);
    dowLayout->addWidget(dowChartView, 1);

    auto* roomTypeLayout = new QVBoxLayout();
    roomTypeLayout->setContentsMargins(0, 0, 0, 0);
    roomTypeLayout->addWidget(createSectionHeader("🛏️ Room types by completed stay", parent));
    auto* roomTypeChartView = new QChartView(m_roomTypeChart);
    roomTypeChartView->setRenderHint(QPainter::Antialiasing);
    roomTypeChartView->setStyleSheet("background-color: #FFFFFF; border-radius: 12px; border: 1px solid #E2E8F0;");
    roomTypeChartView->setMinimumHeight(350);
    roomTypeLayout->addWidget(roomTypeChartView, 1);

    chartsLayout2->addLayout(dowLayout, 6);
    chartsLayout2->addLayout(roomTypeLayout, 4);
    mainLayout->addLayout(chartsLayout2);
}

void StatisticsPageWidget::setupUI() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; } QWidget#scrollWidget { background: transparent; }");

    QWidget* scrollWidget = new QWidget(scrollArea);
    scrollWidget->setObjectName("scrollWidget");
    auto* mainLayout = new QVBoxLayout(scrollWidget);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // Page title.
    auto* pageTitle = new QLabel("Invoice & Booking Statistics", scrollWidget);
    pageTitle->setStyleSheet("font-size: 24px; font-weight: 900; color: #2B3674;");
    mainLayout->addWidget(pageTitle);

    // Chart area.
    setupCharts();
    setupTopChartsUI(scrollWidget, mainLayout);
    setupBottomChartsUI(scrollWidget, mainLayout);

    setupFilterBar(mainLayout);

    // Invoice table area.
    mainLayout->addWidget(createSectionHeader("📋 Invoice List", scrollWidget));
    setupTable();
    m_invoiceTable->setMinimumHeight(400);
    mainLayout->addWidget(m_invoiceTable);

    scrollArea->setWidget(scrollWidget);
    rootLayout->addWidget(scrollArea);
}

void StatisticsPageWidget::setupCharts() {
    // Revenue bar chart.
    m_barChart = new QChart();
    // Modified: localize revenue-axis labels with comma-separated currency grouping.
    m_barChart->setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
    m_barChart->setLocalizeNumbers(true);
    m_barSeries = new QBarSeries();
    m_barChart->addSeries(m_barSeries);
    // The section QLabel already supplies the title.
    m_barChart->setAnimationOptions(QChart::SeriesAnimations);

    m_barAxisX = new QBarCategoryAxis();
    m_barAxisY = new QValueAxis();
    m_barChart->addAxis(m_barAxisX, Qt::AlignBottom);
    m_barChart->addAxis(m_barAxisY, Qt::AlignLeft);
    m_barSeries->attachAxis(m_barAxisX);
    m_barSeries->attachAxis(m_barAxisY);
    m_barChart->legend()->setVisible(true);
    m_barChart->legend()->setAlignment(Qt::AlignBottom);

    // Selected-period revenue donut chart.
    m_pieChart = new QChart();
    m_pieSeries = new QPieSeries();

    m_pieSeries->setHoleSize(0.45); // Display a donut chart.

    m_pieChart->addSeries(m_pieSeries);
    // The section QLabel already supplies the title.
    m_pieChart->setAnimationOptions(QChart::SeriesAnimations);
    m_pieChart->legend()->setAlignment(Qt::AlignBottom);

    // Actual-arrival weekday chart.
    m_dowChart = new QChart();
    m_dowSeries = new QBarSeries();
    m_dowChart->addSeries(m_dowSeries);
    m_dowChart->setAnimationOptions(QChart::SeriesAnimations);

    m_dowAxisX = new QBarCategoryAxis();
    m_dowAxisY = new QValueAxis();
    m_dowChart->addAxis(m_dowAxisX, Qt::AlignBottom);
    m_dowChart->addAxis(m_dowAxisY, Qt::AlignLeft);
    m_dowSeries->attachAxis(m_dowAxisX);
    m_dowSeries->attachAxis(m_dowAxisY);
    m_dowChart->legend()->setVisible(false); // A single series does not need a legend.

    // Room-type distribution chart.
    m_roomTypeChart = new QChart();
    m_roomTypeSeries = new QPieSeries();
    m_roomTypeSeries->setHoleSize(0.45);
    m_roomTypeChart->addSeries(m_roomTypeSeries);
    m_roomTypeChart->setAnimationOptions(QChart::SeriesAnimations);
    m_roomTypeChart->legend()->setAlignment(Qt::AlignBottom);
}

void StatisticsPageWidget::setupFilterBar(QVBoxLayout* parentLayout) {
    auto* filterLayout = new QHBoxLayout();
    filterLayout->setSpacing(15);

    QString commonInputStyle = "background-color: #F4F7FE; border: 1px solid #E9EDF7; border-radius: 10px; padding: 8px 16px; font-size: 13px; color: #2B3674;";
    QString labelStyle = "color: #2B3674; font-size: 13px; font-weight: 600;";

    // Search input.
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Find Invoice ID, Booking...");
    addSearchIcon(m_searchEdit);
    m_searchEdit->setStyleSheet("QLineEdit { " + commonInputStyle + " min-width: 200px; }"
                                "QLineEdit:focus { border: 1px solid #005BFE; }");

    // Amount filter.
    m_amountFilter = new QComboBox(this);
    m_amountFilter->addItems({ "All Price Ranges", "Below 1,000,000 VND", "1,000,000 VND - 5,000,000 VND", "Above 5,000,000 VND" });
    m_amountFilter->setStyleSheet("QComboBox { " + commonInputStyle + " }"
                                  "QComboBox QAbstractItemView { background-color: #FFFFFF; color: #2B3674; selection-background-color: #005BFE; selection-color: #FFFFFF; border: 1px solid #E9EDF7; }");

    // Invoice issue-date filter.
    m_dateFrom = QDate::currentDate().addMonths(-1);
    m_dateTo = QDate::currentDate();
    m_periodButton = new QPushButton("Choose reporting period", this);
    m_periodButton->setStyleSheet("QPushButton { " + commonInputStyle + " font-weight: 700; }"
                                  "QPushButton:hover { border: 1px solid #005BFE; background-color: #EAF2FF; }");
    m_periodSummary = new QLabel(this);
    m_periodSummary->setStyleSheet("color:#6B7FA8; font-size:12px; font-weight:600;");
    m_periodSummary->setText(QString("%1 → %2")
        .arg(m_dateFrom.toString("dd MMM yyyy"), m_dateTo.toString("dd MMM yyyy")));

    QLabel* lblSearch = new QLabel("Search:", this);
    lblSearch->setStyleSheet(labelStyle);
    
    QLabel* lblPeriod = new QLabel("Period:", this);
    lblPeriod->setStyleSheet(labelStyle);

    filterLayout->addWidget(lblSearch);
    filterLayout->addWidget(m_searchEdit);
    filterLayout->addWidget(m_amountFilter);
    filterLayout->addWidget(lblPeriod);
    filterLayout->addWidget(m_periodButton);
    filterLayout->addWidget(m_periodSummary);
    filterLayout->addStretch();

    parentLayout->addLayout(filterLayout);

    // Modified: Rebuild both invoice rows and charts from the same selected filter scope, avoiding a table/chart mismatch.
    connect(m_searchEdit, &QLineEdit::textChanged, this, &StatisticsPageWidget::refreshData);
    connect(m_amountFilter, &QComboBox::currentIndexChanged, this, &StatisticsPageWidget::refreshData);
    connect(m_periodButton, &QPushButton::clicked, this, [this]() {
        // Modified: Statistics now shares the booking-room calendar format while allowing historical inclusive report periods.
        SchedulePickerDialog picker(QDateTime(m_dateFrom, QTime(0, 0)),
                                    QDateTime(m_dateTo, QTime(0, 0)),
                                    {}, this, false, false,
                                    SchedulePickerDialog::Purpose::ReportingPeriod);
        if (picker.exec() != QDialog::Accepted) {
            return;
        }
        m_dateFrom = picker.selectedCheckIn().date();
        m_dateTo = picker.selectedCheckOut().date();
        m_periodSummary->setText(QString("%1 → %2")
            .arg(m_dateFrom.toString("dd MMM yyyy"), m_dateTo.toString("dd MMM yyyy")));
        refreshData();
    });
}

void StatisticsPageWidget::setupTable() {
    m_invoiceTable = new QTableWidget(this);
    m_invoiceTable->setColumnCount(7); // Include the action column.
    // Modified: Present invoice issue time, because revenue statistics are based on issued invoices rather than payment-settlement timing.
    m_invoiceTable->setHorizontalHeaderLabels({ "Invoice ID", "Booking ID", "Invoice Issued", "Room Charge", "Service Charge", "Total Amount", "Actions" });

    for (int i = 0; i < 6; ++i) {
        m_invoiceTable->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
    }
    m_invoiceTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Fixed);
    m_invoiceTable->setColumnWidth(6, 100);

    m_invoiceTable->verticalHeader()->setVisible(false); // Hide the row-number header.
    m_invoiceTable->verticalHeader()->setDefaultSectionSize(42); // Match reservation row height.
    m_invoiceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_invoiceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_invoiceTable->setStyleSheet(
        "QTableWidget { background-color: #FFFFFF; border-radius: 8px; border: 1px solid #E2E8F0; gridline-color: #F1F5F9; }"
        "QHeaderView::section { background-color: #F8FAFC; font-weight: bold; color: #475569; padding: 12px; border: none; border-bottom: 2px solid #E2E8F0; }"
        "QTableWidget::item { padding: 0px 10px; color: #1E293B; }"
    );
}

void StatisticsPageWidget::refreshData() {
    if (!m_manager) return;

    auto invoices = m_manager->getInvoices();
    m_invoiceTable->setRowCount(0);

    std::map<int, double> monthlyRevenue;
    for (int i = 1; i <= 12; ++i) monthlyRevenue[i] = 0.0;
    
    std::map<int, int> dowCount;
    for (int i = 1; i <= 7; ++i) dowCount[i] = 0;
    std::map<QString, int> roomTypeCount;

    double filteredRoomFee = 0.0;
    double filteredServiceFee = 0.0;
    const QString searchText = m_searchEdit->text().trimmed().toLower();
    const int amountFilter = m_amountFilter->currentIndex();
    const QDate fromDate = m_dateFrom;
    const QDate toDate = m_dateTo;

    for (const auto& inv : invoices) {
        if (!inv) continue;

        QString invId = QString::fromStdString(inv->getInvoiceId());
        QString bookId = QString::fromStdString(inv->getBookingId());
        // Modified: Aggregate finance by the immutable invoice issue date so charts never treat planned reservations as revenue.
        QString payDate = QString::fromStdString(inv->getInvoiceIssuedDate());
        if (payDate.isEmpty()) {
            payDate = QString::fromStdString(inv->getPaymentReceivedDate());
        }
        const QDate issuedDate = QDate::fromString(payDate, Qt::ISODate);
        const double totalAmount = inv->calculateTotal();
        const bool matchesSearch = searchText.isEmpty()
            || invId.toLower().contains(searchText) || bookId.toLower().contains(searchText);
        const bool matchesAmount = amountFilter == 0
            || (amountFilter == 1 && totalAmount < 1000000)
            || (amountFilter == 2 && totalAmount >= 1000000 && totalAmount <= 5000000)
            || (amountFilter == 3 && totalAmount > 5000000);
        // Modified: Treat an invalid selected range or invoice date as out of scope instead of displaying inconsistent table and chart data.
        const bool matchesDate = issuedDate.isValid() && fromDate <= toDate
            && issuedDate >= fromDate && issuedDate <= toDate;
        if (!matchesSearch || !matchesAmount || !matchesDate) {
            continue;
        }

        // Modified: Use Invoice's immutable subtotal so time-based bills show their rounded hourly room charge instead of the retired nights × unit-price calculation.
        double roomCharge = inv->calculateSubtotal();
        double serviceCharge = 0.0;

        auto lockedBooking = inv->getBooking();
        // Modified: Weekly arrival statistics use the invoice's actual check-in snapshot, avoiding planned-date inflation.
        QDateTime actualCheckIn = QDateTime::fromString(
            QString::fromStdString(inv->getActualCheckInAtSnapshot()), Qt::ISODateWithMs);
        if (!actualCheckIn.isValid() && lockedBooking) {
            actualCheckIn = QDateTime::fromString(
                QString::fromStdString(lockedBooking->getActualCheckInAt()), Qt::ISODateWithMs);
            if (!actualCheckIn.isValid()) {
                actualCheckIn = QDateTime(
                    QDate::fromString(QString::fromStdString(lockedBooking->getActualCheckInDate()), Qt::ISODate),
                    QTime(0, 0));
            }
        }
        if (actualCheckIn.isValid()) {
            dowCount[actualCheckIn.date().dayOfWeek()]++;
        }

        // Modified: Prefer the immutable room-type snapshot so historic chart data cannot change after a room is edited or archived.
        QString roomType = QString::fromStdString(inv->getRoomTypeSnapshot()).trimmed();
        if (roomType.isEmpty() && lockedBooking && lockedBooking->getRoom()) {
            roomType = QString::fromStdString(lockedBooking->getRoom()->getRoomTypeName());
        }
        if (!roomType.isEmpty()) {
            roomTypeCount[roomType]++;
        }

        // Modified: The table and every chart below now use exactly the same filtered invoice set.
        int row = m_invoiceTable->rowCount();
        m_invoiceTable->insertRow(row);

        auto createCenteredItem = [](const QString& text) {
            auto* item = new QTableWidgetItem(text);
            item->setTextAlignment(Qt::AlignCenter);
            return item;
            };

        m_invoiceTable->setItem(row, 0, createCenteredItem(invId));
        m_invoiceTable->setItem(row, 1, createCenteredItem(bookId));
        m_invoiceTable->setItem(row, 2, createCenteredItem(payDate));
        // Modified: format every invoice amount with comma-separated VND thousands in the statistics table.
        m_invoiceTable->setItem(row, 3, createCenteredItem(formatMoney(roomCharge) + " VND"));
        m_invoiceTable->setItem(row, 4, createCenteredItem(formatMoney(serviceCharge) + " VND"));

        auto* totalItem = createCenteredItem(formatMoney(totalAmount) + " VND");
        totalItem->setData(Qt::UserRole, totalAmount);
        m_invoiceTable->setItem(row, 5, totalItem);

        QWidget* widgetContainer = new QWidget();
        QHBoxLayout* layout = new QHBoxLayout(widgetContainer);

        QPushButton* btnView = new QPushButton("💳", widgetContainer);
        btnView->setToolTip(QString());
        auto* tooltipFilter = new InvoiceActionTooltipFilter(
            btnView, "Invoice's Details", btnView);
        btnView->installEventFilter(tooltipFilter);
        btnView->setCursor(Qt::PointingHandCursor);
        btnView->setMinimumSize(30, 30);
        btnView->setMaximumSize(30, 30);
        btnView->setStyleSheet(
            "QPushButton {"
            "  background-color: #EFF6FF; color: #2563EB; border: 1px solid #BFDBFE;"
            "  border-radius: 8px; font-size: 14px;"
            "  min-width: 30px; max-width: 30px; min-height: 30px; max-height: 30px; padding: 0px;"
            "}"
            "QPushButton:hover {"
            "  background-color: #DBEAFE;"
            "}"
        );

        connect(btnView, &QPushButton::clicked, this, [this, invId]() {
            this->onViewInvoiceClicked(invId);
            });

        layout->addStretch();
        layout->addWidget(btnView);
        layout->addStretch();
        layout->setAlignment(Qt::AlignCenter);
        layout->setContentsMargins(0, 0, 0, 0);
        m_invoiceTable->setCellWidget(row, 6, widgetContainer);

        if (issuedDate.year() == QDate::currentDate().year()) {
            monthlyRevenue[issuedDate.month()] += totalAmount;
        }
        filteredRoomFee += roomCharge;
        filteredServiceFee += serviceCharge;
    }

    m_barSeries->clear();
    m_barAxisX->clear();
    QBarSet* revSet = new QBarSet("Revenue (VNĐ)");
    revSet->setColor(QColor("#005BFE"));
    QStringList months;
    double maxVal = 0;
    for (int i = 1; i <= 12; ++i) {
        *revSet << monthlyRevenue[i];
        months << QString("T%1").arg(i);
        if (monthlyRevenue[i] > maxVal) maxVal = monthlyRevenue[i];
    }
    m_barSeries->append(revSet);
    m_barAxisX->append(months);
    if (maxVal == 0) maxVal = 1000000;
    m_barAxisY->setRange(0, maxVal + (maxVal * 0.1));

    m_pieSeries->clear();
    if (filteredRoomFee > 0 || filteredServiceFee > 0) {
        auto* roomFeeSlice = m_pieSeries->append("Room Fee", filteredRoomFee);
        roomFeeSlice->setColor(QColor("#05CD99"));
        roomFeeSlice->setLabel(QString("Room Fee: %1 VND")
            .arg(formatMoney(filteredRoomFee)));
        auto* serviceFeeSlice = m_pieSeries->append("Service Fee", filteredServiceFee);
        serviceFeeSlice->setColor(QColor("#F59E0B"));
        serviceFeeSlice->setLabel(QString("Service Fee: %1 VND")
            .arg(formatMoney(filteredServiceFee)));
        for (auto slice : m_pieSeries->slices()) {
            slice->setLabelVisible(true);
            slice->setLabelPosition(QPieSlice::LabelOutside);
        }
    }
    else {
        m_pieSeries->append("No Data Available", 1)->setColor(QColor("#E2E8F0"));
    }

    // Update actual-arrival weekday chart.
    m_dowSeries->clear();
    m_dowAxisX->clear();
    QBarSet* dowSet = new QBarSet("Bookings");
    dowSet->setColor(QColor("#8B5CF6")); // Purple.
    QStringList days = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    int maxDow = 0;
    for (int i = 1; i <= 7; ++i) {
        *dowSet << dowCount[i];
        if (dowCount[i] > maxDow) maxDow = dowCount[i];
    }
    m_dowSeries->append(dowSet);
    m_dowAxisX->append(days);
    if (maxDow == 0) maxDow = 10;
    m_dowAxisY->setRange(0, maxDow + (maxDow * 0.2));
    m_dowAxisY->setLabelFormat("%d");

    // Update room-type distribution chart.
    m_roomTypeSeries->clear();
    if (!roomTypeCount.empty()) {
        QList<QColor> colors = {QColor("#EF4444"), QColor("#3B82F6"), QColor("#10B981"), QColor("#F59E0B"), QColor("#6366F1"), QColor("#EC4899")};
        int colorIdx = 0;
        for (const auto& pair : roomTypeCount) {
            auto* slice = m_roomTypeSeries->append(pair.first, pair.second);
            slice->setColor(colors[colorIdx % colors.size()]);
            slice->setLabelVisible(true);
            slice->setLabelPosition(QPieSlice::LabelOutside);
            colorIdx++;
        }
    } else {
        m_roomTypeSeries->append("No Data Available", 1)->setColor(QColor("#E2E8F0"));
    }

}

void StatisticsPageWidget::applyFilters() {
    // Modified: Keep this compatibility slot, but refresh the complete presentation so filters never update only table rows.
    refreshData();
}

void StatisticsPageWidget::onViewInvoiceClicked(const QString& invoiceId) {
    if (!m_manager) return;

    // Resolve the canonical invoice from the backend.
    auto invoices = m_manager->getInvoices();
    for (const auto& inv : invoices) {
        if (inv && QString::fromStdString(inv->getInvoiceId()) == invoiceId) {
            // Use the invoice's immutable HTML snapshot.
            std::string detailsHTML = inv->generateInvoiceDetails();

            // Present the same read-only invoice view used after checkout.
            InvoiceDialog dialog(QString::fromStdString(detailsHTML), this);
            dialog.exec();
            return;
        }
    }
}
