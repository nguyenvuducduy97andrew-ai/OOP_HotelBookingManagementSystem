#include "RoomPageWidget.h"
#include "RoomDialog.h"
#include "StandardRoom.h"
#include "DeluxeRoom.h"
#include "SuiteRoom.h"
#include "CustomSuccessDialog.h"
#include "CustomConfirmDialog.h"
#include "DataManager.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QMessageBox>
#include <QDebug>
#include <QDate>
#include <QPainter>
#include <QLocale>
#include <vector>

namespace {
QString formatMoney(double value)
{
    return QLocale(QLocale::Vietnamese, QLocale::Vietnam).toString(value, 'f', 0);
}
}

RoomPageWidget::RoomPageWidget(HotelManager* manager, QWidget *parent)
    : QWidget(parent), m_manager(manager), m_selectedTypeFilter("All") {
    setupUI();
    refreshData();
}

void setupRoomPageStyle(QWidget* widget) {
    widget->setStyleSheet(R"(
        QWidget#middlePanel {
            background-color: #FFFFFF;
        }
        QListWidget {
            border: none;
            background-color: transparent;
        }
        QListWidget::item {
            background-color: #F8FAFC;
            border: 1px solid #E2E8F0;
            border-radius: 12px;
            margin-bottom: 12px;
            padding: 10px;
        }
        QListWidget::item:selected {
            background-color: #F1F5F9;
            border: 1px solid #005BFE;
            color: #2B3674;
        }
        QLineEdit#searchEdit {
            background-color: #F4F7FE;
            border: 1px solid #E9EDF7;
            border-radius: 10px;
            padding: 8px 16px;
            font-size: 13px;
            color: #2B3674;
        }
        QLineEdit#searchEdit:focus {
            border: 1px solid #005BFE;
        }
        QPushButton#btnAddRoom {
            background-color: #005BFE;
            color: #FFFFFF;
            font-weight: 700;
            border-radius: 10px;
            padding: 8px 18px;
            font-size: 13px;
            border: none;
        }
        QPushButton#btnAddRoom:hover {
            background-color: #2B7BFF;
        }
        QPushButton.filterBtn {
            background-color: #F4F7FE;
            color: #A3AED0;
            font-weight: 600;
            border-radius: 8px;
            padding: 6px 14px;
            border: 1px solid #E9EDF7;
            font-size: 12px;
        }
        QPushButton.filterBtn:hover {
            background-color: #E2E8F0;
            color: #2B3674;
        }
        QPushButton.filterBtn[active="true"] {
            background-color: #005BFE;
            color: #FFFFFF;
            border: 1px solid #005BFE;
        }
        QFrame#detailFrame {
            background-color: #FFFFFF;
            border: 1px solid #E9EDF7;
            border-radius: 20px;
        }
        QLabel#detailTitle {
            font-size: 22px;
            font-weight: 800;
            color: #2B3674;
        }
        QLabel#detailBadgeAvailable {
            background-color: #ECFDF5;
            color: #05CD99;
            border-radius: 6px;
            padding: 4px 10px;
            font-size: 12px;
            font-weight: 700;
        }
        QLabel#detailBadgeOccupied {
            background-color: #FFFBEB;
            color: #D97706;
            border-radius: 6px;
            padding: 4px 10px;
            font-size: 12px;
            font-weight: 700;
        }
        QLabel#detailBadgeMaint {
            background-color: #FEF2F2;
            color: #EF4444;
            border-radius: 6px;
            padding: 4px 10px;
            font-size: 12px;
            font-weight: 700;
        }
        QLabel#detailBadgeAwaiting {
            background-color: #F5F3FF;
            color: #7E22CE;
            border-radius: 6px;
            padding: 4px 10px;
            font-size: 12px;
            font-weight: 700;
        }
        QPushButton#btnEdit {
            background-color: #E9EFFF;
            color: #005BFE;
            font-weight: 700;
            border-radius: 8px;
            padding: 6px 14px;
            border: none;
            font-size: 12px;
        }
        QPushButton#btnEdit:hover {
            background-color: #D6E4FF;
        }
        QPushButton#btnDelete {
            background-color: #FEE2E2;
            color: #EF4444;
            font-weight: 700;
            border-radius: 8px;
            padding: 6px 14px;
            border: none;
            font-size: 12px;
        }
        QPushButton#btnDelete:hover {
            background-color: #FCA5A5;
        }
    )");
}

void RoomPageWidget::setupUI() {
    setupRoomPageStyle(this);

    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(20);

    // -------------------------------------------------------------
    // LEFT/MIDDLE PANEL (Room List)
    // -------------------------------------------------------------
    auto* middlePanel = new QWidget(this);
    middlePanel->setObjectName("middlePanel");
    auto* listLayout = new QVBoxLayout(middlePanel);
    listLayout->setContentsMargins(0, 0, 0, 0);
    listLayout->setSpacing(12);

    // Search and Add Row
    auto* searchRow = new QHBoxLayout();
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setObjectName("searchEdit");
    m_searchEdit->setPlaceholderText("Search rooms...");
    
    m_addRoomBtn = new QPushButton("Add Room", this);
    m_addRoomBtn->setObjectName("btnAddRoom");

    searchRow->addWidget(m_searchEdit);
    searchRow->addWidget(m_addRoomBtn);
    listLayout->addLayout(searchRow);

    // Filter Buttons Row
    auto* filterRow = new QHBoxLayout();
    m_filterAllBtn = new QPushButton("All", this);
    m_filterAllBtn->setProperty("class", "filterBtn");
    m_filterAllBtn->setProperty("active", true);

    m_filterStdBtn = new QPushButton("Standard", this);
    m_filterStdBtn->setProperty("class", "filterBtn");

    m_filterDlxBtn = new QPushButton("Deluxe", this);
    m_filterDlxBtn->setProperty("class", "filterBtn");

    m_filterSuiBtn = new QPushButton("Suite", this);
    m_filterSuiBtn->setProperty("class", "filterBtn");

    filterRow->addWidget(m_filterAllBtn);
    filterRow->addWidget(m_filterStdBtn);
    filterRow->addWidget(m_filterDlxBtn);
    filterRow->addWidget(m_filterSuiBtn);
    filterRow->addStretch();
    listLayout->addLayout(filterRow);

    // Room List
    m_roomListWidget = new QListWidget(this);
    m_roomListWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    listLayout->addWidget(m_roomListWidget);

    mainLayout->addWidget(middlePanel, 3); // 3 parts width

    // -------------------------------------------------------------
    // RIGHT PANEL (Room Detail)
    // -------------------------------------------------------------
    m_detailFrame = new QFrame(this);
    m_detailFrame->setObjectName("detailFrame");
    auto* detailLayout = new QVBoxLayout(m_detailFrame);
    detailLayout->setContentsMargins(20, 20, 20, 20);
    detailLayout->setSpacing(15);

    // Title Row
    auto* detailHeader = new QHBoxLayout();
    auto* titleLabel = new QLabel("Room Details", this);
    titleLabel->setStyleSheet("font-size: 14px; font-weight: 700; color: #A3AED0;");
    m_editRoomBtn = new QPushButton("Edit", this);
    m_editRoomBtn->setObjectName("btnEdit");
    m_deleteRoomBtn = new QPushButton("Delete", this);
    m_deleteRoomBtn->setObjectName("btnDelete");

    detailHeader->addWidget(titleLabel);
    detailHeader->addStretch();
    detailHeader->addWidget(m_editRoomBtn);
    detailHeader->addWidget(m_deleteRoomBtn);
    detailLayout->addLayout(detailHeader);

    // Detail Panel Content (We wrap this in a widget to easily hide/show)
    m_detailPanel = new QWidget(this);
    auto* contentLayout = new QVBoxLayout(m_detailPanel);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(15);

    // Room Name & Status
    auto* nameRow = new QHBoxLayout();
    m_detailTitleLabel = new QLabel("Room 101", this);
    m_detailTitleLabel->setObjectName("detailTitle");
    m_detailStatusLabel = new QLabel("Available", this);
    m_detailStatusLabel->setObjectName("detailBadgeAvailable");
    
    nameRow->addWidget(m_detailTitleLabel);
    nameRow->addWidget(m_detailStatusLabel);
    nameRow->addStretch();
    contentLayout->addLayout(nameRow);

    // Image placeholder
    m_detailImageLabel = new QLabel(this);
    m_detailImageLabel->setMinimumHeight(180);
    m_detailImageLabel->setMaximumHeight(220);
    m_detailImageLabel->setStyleSheet("background-color: #F4F7FE; border-radius: 14px;");
    m_detailImageLabel->setAlignment(Qt::AlignCenter);
    contentLayout->addWidget(m_detailImageLabel);

    // Specs
    auto* specsRow = new QHBoxLayout();
    m_detailSizeLabel = new QLabel("📏 25m²", this);
    m_detailSizeLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #2B3674;");
    m_detailBedLabel = new QLabel("🛏 Queen Bed", this);
    m_detailBedLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #2B3674;");
    m_detailGuestLabel = new QLabel("👤 2 Guests", this);
    m_detailGuestLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #2B3674;");

    specsRow->addWidget(m_detailSizeLabel);
    specsRow->addWidget(m_detailBedLabel);
    specsRow->addWidget(m_detailGuestLabel);
    contentLayout->addLayout(specsRow);

    // Extra Fees details
    m_detailExtraFeeLabel = new QLabel(this);
    m_detailExtraFeeLabel->setStyleSheet("font-size: 13px; font-weight: 700; color: #005BFE; background-color: #E9EFFF; border-radius: 8px; padding: 8px;");
    m_detailExtraFeeLabel->setVisible(false);
    contentLayout->addWidget(m_detailExtraFeeLabel);

    // Description
    auto* descHeader = new QLabel("Room Description", this);
    descHeader->setStyleSheet("font-size: 13px; font-weight: 700; color: #2B3674;");
    m_detailDescLabel = new QLabel(this);
    m_detailDescLabel->setWordWrap(true);
    m_detailDescLabel->setStyleSheet("font-size: 12px; color: #A3AED0; line-height: 18px;");

    contentLayout->addWidget(descHeader);
    contentLayout->addWidget(m_detailDescLabel);

    // Features
    auto* featHeader = new QLabel("Amenities", this);
    featHeader->setStyleSheet("font-size: 13px; font-weight: 700; color: #2B3674;");
    auto* featDesc = new QLabel("✔ Free Wi-Fi  ·  ✔ Smart TV  ·  ✔ Air Conditioning  ·  ✔ Private bathroom", this);
    featDesc->setStyleSheet("font-size: 11px; color: #A3AED0;");
    contentLayout->addWidget(featHeader);
    contentLayout->addWidget(featDesc);

    contentLayout->addStretch();
    detailLayout->addWidget(m_detailPanel);

    mainLayout->addWidget(m_detailFrame, 2); // 2 parts width

    // Connects
    connect(m_searchEdit, &QLineEdit::textChanged, this, &RoomPageWidget::onSearchChanged);
    connect(m_filterAllBtn, &QPushButton::clicked, this, &RoomPageWidget::onFilterTypeSelected);
    connect(m_filterStdBtn, &QPushButton::clicked, this, &RoomPageWidget::onFilterTypeSelected);
    connect(m_filterDlxBtn, &QPushButton::clicked, this, &RoomPageWidget::onFilterTypeSelected);
    connect(m_filterSuiBtn, &QPushButton::clicked, this, &RoomPageWidget::onFilterTypeSelected);
    connect(m_addRoomBtn, &QPushButton::clicked, this, &RoomPageWidget::onAddRoomClicked);
    connect(m_editRoomBtn, &QPushButton::clicked, this, &RoomPageWidget::onEditRoomClicked);
    connect(m_deleteRoomBtn, &QPushButton::clicked, this, &RoomPageWidget::onDeleteRoomClicked);
    connect(m_roomListWidget, &QListWidget::itemSelectionChanged, this, &RoomPageWidget::onRoomSelectionChanged);

    m_detailFrame->setVisible(false);
}

void RoomPageWidget::onSearchChanged(const QString& text) {
    m_searchQuery = text.trimmed();
    refreshData();
}

void RoomPageWidget::onFilterTypeSelected() {
    auto* clickedBtn = qobject_cast<QPushButton*>(sender());
    if (!clickedBtn) return;

    QList<QPushButton*> buttons = {m_filterAllBtn, m_filterStdBtn, m_filterDlxBtn, m_filterSuiBtn};
    for (auto* btn : buttons) {
        btn->setProperty("active", btn == clickedBtn);
        btn->style()->unpolish(btn);
        btn->style()->polish(btn);
    }

    if (clickedBtn == m_filterAllBtn) m_selectedTypeFilter = "All";
    else if (clickedBtn == m_filterStdBtn) m_selectedTypeFilter = "Standard";
    else if (clickedBtn == m_filterDlxBtn) m_selectedTypeFilter = "Deluxe";
    else if (clickedBtn == m_filterSuiBtn) m_selectedTypeFilter = "Suite";

    refreshData();
}

QWidget* RoomPageWidget::createRoomCard(const std::shared_ptr<Room>& room) {
    auto* card = new QWidget();
    auto* layout = new QHBoxLayout(card);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(12);

    // Left Icon/Thumbnail
    auto* thumbnail = new QLabel(card);
    thumbnail->setFixedSize(65, 65);
    thumbnail->setStyleSheet("border-radius: 8px; font-size: 24px; text-align: center;");
    thumbnail->setAlignment(Qt::AlignCenter);

    // Specs
    QString typeLabel;
    QString specLabel;
    QColor typeColor;

    // Modified: Use room metadata accessors for type labels and subtype-specific fees.
    if (room->getRoomTypeName() == "Standard") {
        typeLabel = "Standard";
        specLabel = "25m² · Queen Bed";
        thumbnail->setText("🛏");
        thumbnail->setStyleSheet("background-color: #E2E8F0; border-radius: 8px; font-size: 24px; color: #475569;");
        typeColor = QColor("#64748B");
    } else if (room->getRoomTypeName() == "Deluxe") {
        typeLabel = "Deluxe";
        specLabel = "35m² · King Bed";
        thumbnail->setText("✨");
        thumbnail->setStyleSheet("background-color: #E0F2FE; border-radius: 8px; font-size: 24px; color: #0369A1;");
        typeColor = QColor("#0284C7");
    } else if (room->getRoomTypeName() == "Suite") {
        typeLabel = "Suite";
        specLabel = "55m² · Super King";
        thumbnail->setText("👑");
        thumbnail->setStyleSheet("background-color: #FEF3C7; border-radius: 8px; font-size: 24px; color: #B45309;");
        typeColor = QColor("#D97706");
    }

    auto* textCol = new QVBoxLayout();
    textCol->setSpacing(2);
    
    auto* titleLabel = new QLabel("Room " + QString::fromStdString(room->getRoomNumber()), card);
    titleLabel->setStyleSheet("font-size: 14px; font-weight: 700; color: #2B3674;");
    
    auto* typeBadge = new QLabel(typeLabel, card);
    typeBadge->setStyleSheet(QString("font-size: 11px; font-weight: 700; color: %1;").arg(typeColor.name()));

    auto* specs = new QLabel(specLabel, card);
    specs->setStyleSheet("font-size: 11px; color: #A3AED0;");

    textCol->addWidget(titleLabel);
    textCol->addWidget(typeBadge);
    textCol->addWidget(specs);

    // Right details (status & price)
    auto* rightCol = new QVBoxLayout();
    rightCol->setSpacing(4);
    rightCol->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // Dynamic Occupied status
    const bool isOccupied = isRoomOccupied(room->getRoomNumber());
    const auto awaitingBooking = getAwaitingBooking(room->getRoomNumber());

    auto* statusBadge = new QLabel(card);
    const bool isUnderMaintenance = !room->getIsAvailable()
        || m_manager->isRoomUnderMaintenance(room->getRoomNumber(), QDate::currentDate().toString(Qt::ISODate).toStdString());
    if (isUnderMaintenance) {
        statusBadge->setText("Maintenance");
        statusBadge->setStyleSheet("background-color: #FEF2F2; color: #EF4444; border-radius: 4px; padding: 2px 6px; font-size: 10px; font-weight: 700;");
    } else if (isOccupied) {
        statusBadge->setText("Occupied");
        statusBadge->setStyleSheet("background-color: #FFFBEB; color: #D97706; border-radius: 4px; padding: 2px 6px; font-size: 10px; font-weight: 700;");
    } else if (awaitingBooking) {
        // Modified: Match Room Status by showing a due but unarrived reservation instead of falsely presenting the room as freely available.
        statusBadge->setText("Awaiting");
        statusBadge->setStyleSheet("background-color: #F5F3FF; color: #7E22CE; border-radius: 4px; padding: 2px 6px; font-size: 10px; font-weight: 700;");
    } else {
        statusBadge->setText("Available");
        statusBadge->setStyleSheet("background-color: #ECFDF5; color: #05CD99; border-radius: 4px; padding: 2px 6px; font-size: 10px; font-weight: 700;");
    }

    auto* priceLabel = new QLabel(formatMoney(room->getBasePrice()) + " VND", card);
    priceLabel->setStyleSheet("font-size: 13px; font-weight: 800; color: #005BFE;");

    rightCol->addWidget(statusBadge);
    rightCol->addWidget(priceLabel);

    layout->addWidget(thumbnail);
    layout->addLayout(textCol);
    layout->addStretch();
    layout->addLayout(rightCol);

    return card;
}

void RoomPageWidget::refreshData() {
    m_roomListWidget->clear();
    if (!m_manager) return;

    refreshOccupancyCache();

    for (const auto& room : m_manager->getRooms()) {
        if (!room) continue;

        // Apply type filter
        QString typeStr = QString::fromStdString(room->getRoomTypeName());

        if (m_selectedTypeFilter != "All" && m_selectedTypeFilter != typeStr) {
            continue;
        }

        // Apply search filter
        QString roomNum = QString::fromStdString(room->getRoomNumber());
        if (!m_searchQuery.isEmpty() && !roomNum.contains(m_searchQuery) && !typeStr.toLower().contains(m_searchQuery.toLower())) {
            continue;
        }

        auto* item = new QListWidgetItem(m_roomListWidget);
        item->setSizeHint(QSize(0, 85));
        item->setData(Qt::UserRole, roomNum);

        auto* cardWidget = createRoomCard(room);
        m_roomListWidget->setItemWidget(item, cardWidget);
    }

    if (m_roomListWidget->count() > 0) {
        m_roomListWidget->setCurrentRow(0);
    } else {
        updateDetailPanel(nullptr);
    }
}

void RoomPageWidget::onRoomSelectionChanged() {
    auto* item = m_roomListWidget->currentItem();
    if (!item) {
        updateDetailPanel(nullptr);
        return;
    }

    std::string roomNum = item->data(Qt::UserRole).toString().toStdString();
    auto room = m_manager->findRoomByNumber(roomNum);
    updateDetailPanel(room);
}

void RoomPageWidget::updateDetailPanel(const std::shared_ptr<Room>& room) {
    if (!room) {
        m_detailFrame->setVisible(false);
        m_detailPanel->setVisible(false);
        m_editRoomBtn->setEnabled(false);
        m_deleteRoomBtn->setEnabled(false);
        return;
    }

    m_detailFrame->setVisible(true);
    m_detailPanel->setVisible(true);
    m_editRoomBtn->setEnabled(true);
    m_deleteRoomBtn->setEnabled(true);

    m_detailTitleLabel->setText("Room " + QString::fromStdString(room->getRoomNumber()));

    // Dynamic Occupied status
    const bool isOccupied = isRoomOccupied(room->getRoomNumber());
    const auto awaitingBooking = getAwaitingBooking(room->getRoomNumber());
    std::string occupantName = "";
    if (isOccupied && m_manager) {
        for (const auto& booking : m_manager->getBookings()) {
            if (!booking || booking->isCancelled() || booking->isDeleted() || !booking->getRoom()) {
                continue;
            }
            if (booking->getRoom()->getRoomNumber() != room->getRoomNumber()) {
                continue;
            }
            if (m_manager->getBookingState(*booking) == BookingState::ACTIVE) {
                if (booking->getCustomer()) {
                    occupantName = booking->getCustomer()->getName();
                }
                break;
            }
        }
    }

    const bool isUnderMaintenance = !room->getIsAvailable()
        || m_manager->isRoomUnderMaintenance(room->getRoomNumber(), QDate::currentDate().toString(Qt::ISODate).toStdString());
    if (isUnderMaintenance) {
        m_detailStatusLabel->setText("Maintenance");
        m_detailStatusLabel->setObjectName("detailBadgeMaint");
        m_detailDescLabel->setText("This room is currently under scheduled maintenance. Please do not assign guests at this time.");
    } else if (isOccupied) {
        m_detailStatusLabel->setText("Occupied");
        m_detailStatusLabel->setObjectName("detailBadgeOccupied");
        m_detailDescLabel->setText(QString("Room is currently occupied (%1). Booking details are available in the reservation tab.").arg(QString::fromStdString(occupantName)));
    } else if (awaitingBooking) {
        const auto customer = awaitingBooking->getCustomer();
        const QString guestName = customer ? QString::fromStdString(customer->getName()) : QStringLiteral("the booked guest");
        m_detailStatusLabel->setText("Awaiting check-in");
        m_detailStatusLabel->setObjectName("detailBadgeAwaiting");
        // Modified: Explain that an unarrived reservation still protects the room's inventory for its booked date range.
        m_detailDescLabel->setText(QString("Awaiting check-in for %1. The room remains reserved and cannot be assigned to another booking for this stay.").arg(guestName));
    } else {
        m_detailStatusLabel->setText("Available");
        m_detailStatusLabel->setObjectName("detailBadgeAvailable");
        m_detailDescLabel->setText("This room is available for a booking during eligible dates.");
    }
    // Refresh label styles to apply name changes
    m_detailStatusLabel->style()->unpolish(m_detailStatusLabel);
    m_detailStatusLabel->style()->polish(m_detailStatusLabel);

    const std::string typeName = room->getRoomTypeName();
    if (typeName == "Standard") {
        m_detailSizeLabel->setText("📏 25m²");
        m_detailBedLabel->setText("🛏 Queen Bed");
        m_detailGuestLabel->setText(QString("👤 %1 Guests").arg(room->getMaximumGuests()));
        m_detailExtraFeeLabel->setVisible(false);
        m_detailImageLabel->setText("🏨 Standard Room Image Placeholder");
        m_detailImageLabel->setStyleSheet("background-color: #F4F7FE; border-radius: 14px; font-weight: bold; color: #A3AED0;");
    } else if (typeName == "Deluxe") {
        m_detailSizeLabel->setText("📏 35m²");
        m_detailBedLabel->setText("🛏 King Bed");
        m_detailGuestLabel->setText(QString("👤 %1 Guests").arg(room->getMaximumGuests()));
        m_detailExtraFeeLabel->setText(QString("💰 Mini Bar Fee: %1 VND").arg(formatMoney(room->getExtraFeeAmount())));
        m_detailExtraFeeLabel->setVisible(true);
        m_detailImageLabel->setText("✨ Deluxe Room Image Placeholder");
        m_detailImageLabel->setStyleSheet("background-color: #E0F2FE; border-radius: 14px; font-weight: bold; color: #0284C7;");
    } else if (typeName == "Suite") {
        m_detailSizeLabel->setText("📏 55m²");
        m_detailBedLabel->setText("🛏 Super King Bed");
        m_detailGuestLabel->setText(QString("👤 %1 Guests").arg(room->getMaximumGuests()));
        m_detailExtraFeeLabel->setText(QString("👑 Premium Service Fee: %1 VND").arg(formatMoney(room->getExtraFeeAmount())));
        m_detailExtraFeeLabel->setVisible(true);
        m_detailImageLabel->setText("👑 Suite Room Image Placeholder");
        m_detailImageLabel->setStyleSheet("background-color: #FEF3C7; border-radius: 14px; font-weight: bold; color: #D97706;");
    }
}

void RoomPageWidget::refreshOccupancyCache()
{
    m_occupiedRoomNumbers.clear();
    if (!m_manager) {
        return;
    }

    for (const auto& booking : m_manager->getBookings()) {
        if (!booking || booking->isCancelled() || booking->isDeleted()) {
            continue;
        }

        const auto room = booking->getRoom();
        if (!room) {
            continue;
        }

        if (m_manager->getBookingState(*booking) == BookingState::ACTIVE) {
            m_occupiedRoomNumbers.insert(room->getRoomNumber());
        }
    }
}

bool RoomPageWidget::isRoomOccupied(const std::string& roomNumber) const
{
    return m_occupiedRoomNumbers.find(roomNumber) != m_occupiedRoomNumbers.end();
}

std::shared_ptr<Booking> RoomPageWidget::getAwaitingBooking(const std::string& roomNumber) const
{
    if (!m_manager) {
        return nullptr;
    }

    const std::string today = QDate::currentDate().toString(Qt::ISODate).toStdString();
    for (const auto& booking : m_manager->getBookings()) {
        if (!booking || booking->isDeleted() || booking->isCancelled() || !booking->getRoom()) {
            continue;
        }
        if (booking->getRoom()->getRoomNumber() == roomNumber
            && m_manager->getBookingState(*booking) == BookingState::UPCOMING
            && booking->getCheckInDate() <= today && today < booking->getCheckOutDate()) {
            return booking;
        }
    }
    return nullptr;
}

void RoomPageWidget::onAddRoomClicked() {
    RoomDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        std::string roomNum = dialog.getRoomNumber().toStdString();
        double price = dialog.getBasePrice();
        RoomType type = dialog.getRoomType();
        double extraFee = dialog.getExtraFee();

        if (m_manager->roomNumberExists(roomNum)) {
            QMessageBox::warning(this, "Add room error", "The room number already exists in the system.");
            return;
        }

        std::string errMsg;
        // Modified: Set subtype fees through the room interface instead of concrete casts.
        if (m_manager->registerRoom(type, roomNum, price, errMsg)) {
            if (!m_manager->updateRoomPricing(roomNum, price, extraFee, errMsg)) {
                std::string discardError;
                m_manager->deleteRoom(roomNum, discardError);
                QMessageBox::warning(
                    this,
                    "Room pricing error",
                    QString::fromStdString(errMsg));
                return;
            }
            if (dialog.shouldScheduleMaintenance() &&
                !m_manager->scheduleRoomMaintenance(roomNum,
                    dialog.getMaintenanceStartDate().toStdString(),
                    dialog.getMaintenanceEndDate().toStdString(),
                    dialog.getMaintenanceNote().toStdString(), errMsg)) {
                std::string discardError;
                m_manager->deleteRoom(roomNum, discardError);
                QMessageBox::warning(this, "Maintenance schedule conflict", QString::fromStdString(errMsg));
                return;
            }
            // Modified: Commit room creation immediately and recover the last saved state on failure.
            if (!DataManager::getInstance().commitChanges(*m_manager)) {
                refreshData();
                QMessageBox::critical(this, "Save Room Failed", "The room was not saved. The previous database state has been restored.");
                return;
            }
            refreshData();
            CustomSuccessDialog("Room added successfully.", this).exec();
        } else {
            QMessageBox::critical(this, "Add room error", QString::fromStdString(errMsg));
        }
    }
}

void RoomPageWidget::onEditRoomClicked() {
    auto* item = m_roomListWidget->currentItem();
    if (!item) return;

    std::string roomNum = item->data(Qt::UserRole).toString().toStdString();
    auto room = m_manager->findRoomByNumber(roomNum);
    if (!room) return;

    // Modified: Read the room type and extra fee from polymorphic accessors.
    RoomType type = RoomType::Standard;
    const std::string typeName = room->getRoomTypeName();
    if (typeName == "Deluxe") {
        type = RoomType::Deluxe;
    } else if (typeName == "Suite") {
        type = RoomType::Suite;
    }
    double extraFee = room->getExtraFeeAmount();

    RoomDialog dialog(QString::fromStdString(roomNum), room->getBasePrice(), type, extraFee, room->getIsAvailable(), this);
    std::vector<RoomMaintenance> existingSchedules;
    const std::string today = QDate::currentDate().toString(Qt::ISODate).toStdString();
    for (const RoomMaintenance& maintenance : m_manager->getRoomMaintenances()) {
        if (maintenance.getRoomNumber() == roomNum && maintenance.getEndDate() > today) {
            existingSchedules.push_back(maintenance);
        }
    }
    dialog.setExistingMaintenanceSchedules(existingSchedules, m_manager->getMaintenanceGuestNotices());
    if (dialog.exec() == QDialog::Accepted) {
        double newPrice = dialog.getBasePrice();
        double newExtraFee = dialog.getExtraFee();
        const bool scheduleMaintenance = dialog.shouldScheduleMaintenance();

        std::string errorMessage;
        if (!dialog.getMaintenanceIdToCancel().isEmpty() &&
            !m_manager->cancelRoomMaintenance(dialog.getMaintenanceIdToCancel().toStdString(), errorMessage)) {
            QMessageBox::warning(this, "Maintenance cancellation failed", QString::fromStdString(errorMessage));
            return;
        }
        if (!dialog.getMaintenanceIdToConfirm().isEmpty() &&
            !m_manager->confirmRoomMaintenance(dialog.getMaintenanceIdToConfirm().toStdString(), errorMessage)) {
            QMessageBox::warning(this, "Maintenance case still pending", QString::fromStdString(errorMessage));
            return;
        }
        if (scheduleMaintenance && !m_manager->scheduleRoomMaintenance(roomNum,
                dialog.getMaintenanceStartDate().toStdString(),
                dialog.getMaintenanceEndDate().toStdString(),
                dialog.getMaintenanceNote().toStdString(), errorMessage)) {
            QMessageBox::warning(this, "Maintenance schedule conflict", QString::fromStdString(errorMessage));
            return;
        }
        const QString maintenanceWorkflowMessage = QString::fromStdString(errorMessage);
        if (!scheduleMaintenance && !room->getIsAvailable() &&
            !m_manager->setRoomAvailability(roomNum, true, errorMessage)) {
            QMessageBox::warning(this, "Room status unavailable", QString::fromStdString(errorMessage));
            return;
        }


        // Modified: Schedule maintenance by date range so future bookings remain valid outside the closure interval.
        if (!m_manager->updateRoomPricing(
                roomNum, newPrice, newExtraFee, errorMessage)) {
            QMessageBox::warning(
                this,
                "Room update failed",
                QString::fromStdString(errorMessage));
            return;
        }

        if (!DataManager::getInstance().commitChanges(*m_manager)) {
            refreshData();
            QMessageBox::critical(this, "Save Room Failed", "The room changes were not saved. The previous database state has been restored.");
            return;
        }

        refreshData();
        // Modified: Make a simulated guest-contact case visible to staff instead of silently treating it as confirmed maintenance.
        CustomSuccessDialog(maintenanceWorkflowMessage.isEmpty()
                                ? "Room information updated successfully."
                                : maintenanceWorkflowMessage,
                            this).exec();
    }
}

void RoomPageWidget::onDeleteRoomClicked() {
    auto* item = m_roomListWidget->currentItem();
    if (!item) return;

    std::string roomNum = item->data(Qt::UserRole).toString().toStdString();
    
    CustomConfirmDialog dialog("Confirm delete", QString("Are you sure you want to delete room %1 from the system?").arg(QString::fromStdString(roomNum)), true, this);
    if (dialog.exec() == QDialog::Accepted && dialog.isConfirmed()) {
        std::string errMsg;
        if (m_manager->deleteRoom(roomNum, errMsg)) {
            if (!DataManager::getInstance().commitChanges(*m_manager)) {
                refreshData();
                QMessageBox::critical(this, "Delete Room Failed", "The room was not deleted because the database could not be updated.");
                return;
            }
            refreshData();
            CustomSuccessDialog("Room deleted successfully.", this).exec();
        } else {
            QMessageBox::critical(this, "Delete room error",
                QString("Could not delete room: %1").arg(QString::fromStdString(errMsg)));
        }
    }
}
