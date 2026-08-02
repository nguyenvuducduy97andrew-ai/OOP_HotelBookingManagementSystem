#include "StatisticsPageWidget.h"
#include "InvoiceDialog.h"
#include "StandardRoom.h"
#include "DeluxeRoom.h"
#include "SuiteRoom.h"
#include <QHeaderView>
#include <QDateTime>
#include <QMessageBox>
#include <QScrollArea>
#include <QCalendarWidget>
#include <QEvent>
#include <QFontMetrics>
#include <QFrame>
#include <QGuiApplication>
#include <QScreen>
#include <map>

namespace {
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

const char* calendarPopupStyle = R"(
    QCalendarWidget QWidget {
        background-color: #FFFFFF;
        color: #2B3674;
    }
    QCalendarWidget QAbstractItemView:enabled {
        color: #2B3674;
        background-color: #FFFFFF;
        selection-background-color: #005BFE;
        selection-color: #FFFFFF;
    }
    QCalendarWidget QAbstractItemView:disabled {
        color: #A3AED0;
    }
    QCalendarWidget QToolButton {
        color: #2B3674;
        background-color: transparent;
    }
    QCalendarWidget QMenu {
        background-color: #FFFFFF;
        color: #2B3674;
    }
)";
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
    barLayout->addWidget(createSectionHeader("📈 Montly Revenue", parent));
    auto* barChartView = new QChartView(m_barChart);
    barChartView->setRenderHint(QPainter::Antialiasing);
    barChartView->setStyleSheet("background-color: #FFFFFF; border-radius: 12px; border: 1px solid #E2E8F0;");
    barChartView->setMinimumHeight(350);
    barLayout->addWidget(barChartView, 1);

    auto* pieLayout = new QVBoxLayout();
    pieLayout->setContentsMargins(0, 0, 0, 0);
    pieLayout->addWidget(createSectionHeader("🍩 Today's Revenue Structure", parent));
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
    dowLayout->addWidget(createSectionHeader("📅 Weekly Bookings Quantity", parent));
    auto* dowChartView = new QChartView(m_dowChart);
    dowChartView->setRenderHint(QPainter::Antialiasing);
    dowChartView->setStyleSheet("background-color: #FFFFFF; border-radius: 12px; border: 1px solid #E2E8F0;");
    dowChartView->setMinimumHeight(350);
    dowLayout->addWidget(dowChartView, 1);

    auto* roomTypeLayout = new QVBoxLayout();
    roomTypeLayout->setContentsMargins(0, 0, 0, 0);
    roomTypeLayout->addWidget(createSectionHeader("🛏️ Most Booked Rooms", parent));
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

    // Tiêu đề trang
    auto* pageTitle = new QLabel("Invoice & Booking Statistics", scrollWidget);
    pageTitle->setStyleSheet("font-size: 24px; font-weight: 900; color: #2B3674;");
    mainLayout->addWidget(pageTitle);

    // Khu vực Biểu đồ
    setupCharts();
    setupTopChartsUI(scrollWidget, mainLayout);
    setupBottomChartsUI(scrollWidget, mainLayout);

    setupFilterBar(mainLayout);

    // Khu vực Bảng dữ liệu (Bottom)
    mainLayout->addWidget(createSectionHeader("📋 Invoice List", scrollWidget));
    setupTable();
    m_invoiceTable->setMinimumHeight(400);
    mainLayout->addWidget(m_invoiceTable);

    scrollArea->setWidget(scrollWidget);
    rootLayout->addWidget(scrollArea);
}

void StatisticsPageWidget::setupCharts() {
    // --- Thiết lập Biểu đồ Cột ---
    m_barChart = new QChart();
    m_barSeries = new QBarSeries();
    m_barChart->addSeries(m_barSeries);
    // Bỏ title mặc định của QChart vì đã dùng QLabel
    m_barChart->setAnimationOptions(QChart::SeriesAnimations);

    m_barAxisX = new QBarCategoryAxis();
    m_barAxisY = new QValueAxis();
    m_barChart->addAxis(m_barAxisX, Qt::AlignBottom);
    m_barChart->addAxis(m_barAxisY, Qt::AlignLeft);
    m_barSeries->attachAxis(m_barAxisX);
    m_barSeries->attachAxis(m_barAxisY);
    m_barChart->legend()->setVisible(true);
    m_barChart->legend()->setAlignment(Qt::AlignBottom);

    // --- Thiết lập Biểu đồ Tròn ---
    m_pieChart = new QChart();
    m_pieSeries = new QPieSeries();

    m_pieSeries->setHoleSize(0.45); // <--- Điểm nhấn làm thành Donut Chart rỗng ruột

    m_pieChart->addSeries(m_pieSeries);
    // Bỏ title mặc định của QChart vì đã dùng QLabel
    m_pieChart->setAnimationOptions(QChart::SeriesAnimations);
    m_pieChart->legend()->setAlignment(Qt::AlignBottom);

    // --- Thiết lập Biểu đồ Thứ ---
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
    m_dowChart->legend()->setVisible(false); // Không cần legend cho cột đơn giản

    // --- Thiết lập Biểu đồ Loại Phòng ---
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

    // Ô tìm kiếm
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Find Invoice ID, Booking...");
    m_searchEdit->setStyleSheet("QLineEdit { " + commonInputStyle + " min-width: 200px; }"
                                "QLineEdit:focus { border: 1px solid #005BFE; }");

    // Lọc theo khoảng giá
    m_amountFilter = new QComboBox(this);
    m_amountFilter->addItems({ "All Price Ranges", "Below 1,000,000 VND", "1,000,000 VND - 5,000,000 VND", "Above 5,000,000 VND" });
    m_amountFilter->setStyleSheet("QComboBox { " + commonInputStyle + " }"
                                  "QComboBox QAbstractItemView { background-color: #FFFFFF; color: #2B3674; selection-background-color: #005BFE; selection-color: #FFFFFF; border: 1px solid #E9EDF7; }");

    // Lọc theo ngày
    m_dateFrom = new QDateEdit(QDate::currentDate().addMonths(-1), this); // Mặc định từ 1 tháng trước
    m_dateFrom->setCalendarPopup(true);
    m_dateFrom->setStyleSheet("QDateEdit { " + commonInputStyle + " }");
    m_dateFrom->calendarWidget()->setStyleSheet(calendarPopupStyle);

    m_dateTo = new QDateEdit(QDate::currentDate(), this);
    m_dateTo->setCalendarPopup(true);
    m_dateTo->setStyleSheet("QDateEdit { " + commonInputStyle + " }");
    m_dateTo->calendarWidget()->setStyleSheet(calendarPopupStyle);

    QLabel* lblSearch = new QLabel("Search:", this);
    lblSearch->setStyleSheet(labelStyle);
    
    QLabel* lblFrom = new QLabel("From:", this);
    lblFrom->setStyleSheet(labelStyle);

    QLabel* lblTo = new QLabel("To:", this);
    lblTo->setStyleSheet(labelStyle);

    filterLayout->addWidget(lblSearch);
    filterLayout->addWidget(m_searchEdit);
    filterLayout->addWidget(m_amountFilter);
    filterLayout->addWidget(lblFrom);
    filterLayout->addWidget(m_dateFrom);
    filterLayout->addWidget(lblTo);
    filterLayout->addWidget(m_dateTo);
    filterLayout->addStretch();

    parentLayout->addLayout(filterLayout);

    // Kết nối sự kiện lọc
    connect(m_searchEdit, &QLineEdit::textChanged, this, &StatisticsPageWidget::applyFilters);
    connect(m_amountFilter, &QComboBox::currentIndexChanged, this, &StatisticsPageWidget::applyFilters);
    connect(m_dateFrom, &QDateEdit::dateChanged, this, &StatisticsPageWidget::applyFilters);
    connect(m_dateTo, &QDateEdit::dateChanged, this, &StatisticsPageWidget::applyFilters);
}

void StatisticsPageWidget::setupTable() {
    m_invoiceTable = new QTableWidget(this);
    m_invoiceTable->setColumnCount(7); // Thêm cột thứ 7 cho nút Hành động
    m_invoiceTable->setHorizontalHeaderLabels({ "Invoice ID", "Booking ID", "Payment Date", "Room Charge", "Service Charge", "Total Amount", "Actions" });

    for (int i = 0; i < 6; ++i) {
        m_invoiceTable->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
    }
    m_invoiceTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Fixed);
    m_invoiceTable->setColumnWidth(6, 100);

    m_invoiceTable->verticalHeader()->setVisible(false); // Xóa cột số thứ tự màu đen bên trái
    m_invoiceTable->verticalHeader()->setDefaultSectionSize(42); // Đặt kích cỡ dòng giống bên Reservations
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

    double todayRoomFee = 0.0;
    double todayServiceFee = 0.0;
    QString todayStr = QDate::currentDate().toString("yyyy-MM-dd");

    for (const auto& inv : invoices) {
        if (!inv) continue;

        QString invId = QString::fromStdString(inv->getInvoiceId());
        QString bookId = QString::fromStdString(inv->getBookingId());
        QString payDate = QString::fromStdString(inv->getPaymentReceivedDate());

        double roomCharge = 0.0;
        double serviceCharge = 0.0;

        auto lockedBooking = inv->getBooking();
        if (lockedBooking) {
            QDate checkIn = QDate::fromString(QString::fromStdString(lockedBooking->getCheckInDate()), "yyyy-MM-dd");
            if (checkIn.isValid()) {
                dowCount[checkIn.dayOfWeek()]++;
            }

            auto lockedRoom = lockedBooking->getRoom();
            if (lockedRoom) {
                roomCharge = inv->getNights() * lockedRoom->calculateTargetPrice();
                QString rType = "Standard";
                if (dynamic_cast<DeluxeRoom*>(lockedRoom.get())) rType = "Deluxe"; //dynamic_cast để ép kiểu con trỏ an toàn 
                else if (dynamic_cast<SuiteRoom*>(lockedRoom.get())) rType = "Suite";
                roomTypeCount[rType]++;
            }
        }

        double totalAmount = inv->calculateTotal();

        // Thêm vào bảng
        int row = m_invoiceTable->rowCount();
        m_invoiceTable->insertRow(row);

        // Căn giữa text cho đẹp
        auto createCenteredItem = [](const QString& text) {
            auto* item = new QTableWidgetItem(text);
            item->setTextAlignment(Qt::AlignCenter);
            return item;
            };

        m_invoiceTable->setItem(row, 0, createCenteredItem(invId));
        m_invoiceTable->setItem(row, 1, createCenteredItem(bookId));
        m_invoiceTable->setItem(row, 2, createCenteredItem(payDate));
        m_invoiceTable->setItem(row, 3, createCenteredItem(QString::number(roomCharge, 'f', 0) + " đ"));
        m_invoiceTable->setItem(row, 4, createCenteredItem(QString::number(serviceCharge, 'f', 0) + " đ"));

        // Lưu giá trị số thực vào cột tổng cộng để lát nữa bộ lọc tính toán dễ hơn
        auto* totalItem = createCenteredItem(QString::number(totalAmount, 'f', 0) + " đ");
        totalItem->setData(Qt::UserRole, totalAmount); // Lưu ngầm data dạng số
        m_invoiceTable->setItem(row, 5, totalItem);

        // Nhúng nút vào bảng
        QWidget* widgetContainer = new QWidget();
        QHBoxLayout* layout = new QHBoxLayout(widgetContainer);

        // Tạo nút "Xem Hóa Đơn"
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

        // Bắt sự kiện nút bấm truyền id hóa đơn
        connect(btnView, &QPushButton::clicked, this, [this, invId]() {
            this->onViewInvoiceClicked(invId);
            });

        layout->addStretch();
        layout->addWidget(btnView);
        layout->addStretch();
        layout->setAlignment(Qt::AlignCenter);
        layout->setContentsMargins(0, 0, 0, 0);
        m_invoiceTable->setCellWidget(row, 6, widgetContainer);

        // Tính toán biểu đồ
        QDate date = QDate::fromString(payDate, "yyyy-MM-dd");
        if (date.isValid() && date.year() == QDate::currentDate().year()) {
            monthlyRevenue[date.month()] += totalAmount;
        }
        if (payDate == todayStr) {
            todayRoomFee += roomCharge;
            todayServiceFee += serviceCharge;
        }
    }

    // (Code cập nhật 2 Biểu đồ m_barSeries và m_pieSeries giữ y chang như phiên bản trước)
    // ...
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
    if (todayRoomFee > 0 || todayServiceFee > 0) {
        m_pieSeries->append("Room Fee", todayRoomFee)->setColor(QColor("#05CD99"));
        m_pieSeries->append("Service Fee", todayServiceFee)->setColor(QColor("#F59E0B"));
        for (auto slice : m_pieSeries->slices()) {
            slice->setLabelVisible(true);
            slice->setLabelPosition(QPieSlice::LabelOutside);
        }
    }
    else {
        m_pieSeries->append("No Data Available", 1)->setColor(QColor("#E2E8F0"));
    }

    // --- Cập nhật Biểu đồ Thứ ---
    m_dowSeries->clear();
    m_dowAxisX->clear();
    QBarSet* dowSet = new QBarSet("Bookings");
    dowSet->setColor(QColor("#8B5CF6")); // Tím
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

    // --- Cập nhật Biểu đồ Loại Phòng ---
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

    applyFilters();
}

void StatisticsPageWidget::applyFilters() {
    QString searchTxt = m_searchEdit->text().toLower();
    int amountIdx = m_amountFilter->currentIndex();
    QDate fromDate = m_dateFrom->date();
    QDate toDate = m_dateTo->date();

    for (int row = 0; row < m_invoiceTable->rowCount(); ++row) {
        bool match = true;

        QString invId = m_invoiceTable->item(row, 0)->text().toLower();
        QString bookId = m_invoiceTable->item(row, 1)->text().toLower();
        if (!searchTxt.isEmpty() && !invId.contains(searchTxt) && !bookId.contains(searchTxt)) {
            match = false;
        }

        double totalAmount = m_invoiceTable->item(row, 5)->data(Qt::UserRole).toDouble();
        if (amountIdx == 1 && totalAmount >= 1000000) match = false; // Dưới 1tr
        if (amountIdx == 2 && (totalAmount < 1000000 || totalAmount > 5000000)) match = false; // 1tr - 5tr
        if (amountIdx == 3 && totalAmount <= 5000000) match = false; // Trên 5tr

        // 3. Lọc Ngày tháng
        QDate payDate = QDate::fromString(m_invoiceTable->item(row, 2)->text(), "yyyy-MM-dd");
        if (payDate.isValid() && (payDate < fromDate || payDate > toDate)) {
            match = false;
        }

        // Hiện/Ẩn dòng dựa trên kết quả lọc
        m_invoiceTable->setRowHidden(row, !match);
    }
}

void StatisticsPageWidget::onViewInvoiceClicked(const QString& invoiceId) {
    if (!m_manager) return;

    // Tìm hóa đơn thật trong Backend
    auto invoices = m_manager->getInvoices();
    for (const auto& inv : invoices) {
        if (inv && QString::fromStdString(inv->getInvoiceId()) == invoiceId) {
            // Lấy chuỗi HTML chi tiết từ class Invoice của bạn
            std::string detailsHTML = inv->generateInvoiceDetails();

            // Hiển thị lên InvoiceDialog giống như khi check-out
            InvoiceDialog dialog(QString::fromStdString(detailsHTML), this);
            dialog.exec();
            return;
        }
    }
}
