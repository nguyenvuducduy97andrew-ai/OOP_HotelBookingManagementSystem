#include "RoomPageWidget.h"
#include "RoomImageCarousel.h"
#include "RoomDialog.h"
#include "StandardRoom.h"
#include "DeluxeRoom.h"
#include "SuiteRoom.h"
#include "CustomSuccessDialog.h"
#include "CustomConfirmDialog.h"
#include "DataManager.h"
#include "SearchFieldUi.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QMessageBox>
#include <QDebug>
#include <QDate>
#include <QDateTime>
#include <QTime>
#include <QPainter>
#include <QLocale>
#include <vector>
#include <unordered_map>

namespace {
QString formatMoney(double value)
{
    // Modified: Format room prices directly in the existing view using comma thousands separators.
    return QLocale(QLocale::English, QLocale::UnitedStates).toString(value, 'f', 0);
}

QString maintenanceEndExclusive(const QString& selectedInclusiveEnd)
{
    const QDate endDate = QDate::fromString(selectedInclusiveEnd, Qt::ISODate);
    // Modified: Store Maintenance as a half-open full-day interval, so a user-selected end date remains unavailable through that entire day.
    return endDate.isValid() ? endDate.addDays(1).toString(Qt::ISODate) : selectedInclusiveEnd;
}

bool hasActiveCleaning(const HotelManager* manager, const std::string& roomNumber, const QDateTime& at)
{
    if (!manager) {
        return false;
    }
    for (const RoomMaintenance& block : manager->getRoomMaintenances()) {
        if (!block.isCleaning() || !block.isConfirmed() || block.getRoomNumber() != roomNumber) {
            continue;
        }
        const QDateTime start = QDateTime::fromString(QString::fromStdString(block.getStartAt()), Qt::ISODateWithMs);
        const QDateTime plannedEnd = QDateTime::fromString(QString::fromStdString(block.getEndAt()), Qt::ISODateWithMs);
        const QDateTime completedAt = QDateTime::fromString(QString::fromStdString(block.getCompletedAt()), Qt::ISODateWithMs);
        const QDateTime effectiveEnd = completedAt.isValid() ? completedAt : plannedEnd;
        if (start.isValid() && effectiveEnd.isValid() && start <= at && at < effectiveEnd) {
            return true;
        }
    }
    return false;
}
}

RoomPageWidget::RoomPageWidget(HotelManager* manager, QWidget *parent)
    : QWidget(parent), m_manager(manager), m_selectedTypeFilter("All") {
    setupUI();
    refreshData();
    m_statusRefreshTimer = new QTimer(this);
    m_statusRefreshTimer->setInterval(30 * 1000);
    // Modified: Refresh visible room status periodically so an expired Cleaning block returns to Available without requiring manual navigation.
    connect(m_statusRefreshTimer, &QTimer::timeout, this, [this]() {
        if (isVisible() && roomStateSignature() != m_lastRoomStateSignature) {
            refreshData();
        }
    });
    m_statusRefreshTimer->start();
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
        QListWidget::item:hover {
            background-color: #F8FAFF;
            border: none;
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
            border: 1px solid #3B58FF;
        }
        QPushButton#btnAddRoom {
            background-color: #3B58FF;
            color: #FFFFFF;
            border: none;
            border-radius: 8px;
            padding: 8px 20px;
            font-weight: 600;
        }
        QPushButton#btnAddRoom:hover { background-color: #4F6BFF; }
        QPushButton.filterBtn {
            background-color: #F4F7FE;
            color: #A3AED0;
            font-weight: 600;
            border-radius: 8px;
            padding: 6px 14px;
            border: 1px solid #E9EDF7;
            font-size: 12px;
        }
        QPushButton[typeFilterBtn="true"] {
            background-color: #F8FAFC;
            color: #475569;
            border-radius: 8px;
            padding: 8px 16px;
            font-weight: 600;
            border: 1px solid transparent;
        }
        QPushButton[typeFilterBtn="true"]:hover {
            background-color: #E2E8F0;
        }
        QPushButton[typeFilterBtn="true"]:checked {
            background-color: #EAF2FF;
            color: #3B58FF;
            border: 1px solid #3B58FF;
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
        QComboBox QAbstractItemView::item:selected {
            background-color: #EAF2FF;
            color: #3B58FF;
        }
        QPushButton#btnEdit {
            background-color: #E9EFFF;
            color: #3B58FF;
            font-weight: 700;
            border-radius: 8px;
            padding: 6px 14px;
            border: none;
            font-size: 12px;
        }
        QPushButton#btnEdit:hover {
            background-color: #D6E4FF;
        }
        QPushButton#btnReady {
            background-color: #DCFCE7;
            color: #15803D;
            font-weight: 700;
            border-radius: 8px;
            padding: 6px 14px;
            border: none;
            font-size: 12px;
        }
        QPushButton#btnReady:hover {
            background-color: #BBF7D0;
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
    addSearchIcon(m_searchEdit);
    
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
    m_markRoomReadyBtn = new QPushButton("Mark room ready", this);
    m_markRoomReadyBtn->setObjectName("btnReady");
    m_markRoomReadyBtn->setVisible(false);
    m_deleteRoomBtn = new QPushButton("Delete", this);
    m_deleteRoomBtn->setObjectName("btnDelete");

    detailHeader->addWidget(titleLabel);
    detailHeader->addStretch();
    detailHeader->addWidget(m_markRoomReadyBtn);
    detailHeader->addWidget(m_editRoomBtn);
    detailHeader->addWidget(m_deleteRoomBtn);
    detailLayout->addLayout(detailHeader);

    // Detail Panel Content (We wrap this in a widget to easily hide/show)
    QScrollArea* detailScrollArea = new QScrollArea(this);
    detailScrollArea->setWidgetResizable(true);
    detailScrollArea->setFrameShape(QFrame::NoFrame);
    detailScrollArea->setStyleSheet("QScrollArea { background-color: transparent; } QWidget#detailPanel { background-color: transparent; }");
    
    m_detailPanel = new QWidget(detailScrollArea);
    m_detailPanel->setObjectName("detailPanel");
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
    m_detailRoomImageCarousel = new RoomImageCarousel(180, m_detailPanel);
    m_detailRoomImageCarousel->hide();
    contentLayout->addWidget(m_detailRoomImageCarousel);

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
    m_detailExtraFeeLabel->setStyleSheet("font-size: 13px; font-weight: 700; color: #3B58FF; background-color: #EAF2FF; border-radius: 8px; padding: 8px;");
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
    m_detailAmenitiesLabel = new QLabel(this);
    m_detailAmenitiesLabel->setStyleSheet("font-size: 12px; color: #A3AED0; line-height: 18px;");
    m_detailAmenitiesLabel->setWordWrap(true);
    contentLayout->addWidget(featHeader);
    contentLayout->addWidget(m_detailAmenitiesLabel);

    contentLayout->addStretch();
    detailScrollArea->setWidget(m_detailPanel);
    detailLayout->addWidget(detailScrollArea);

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
    connect(m_markRoomReadyBtn, &QPushButton::clicked, this, &RoomPageWidget::onMarkRoomReadyClicked);
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
        specLabel = QString("%1m² · %2").arg(room->getArea()).arg(QString::fromStdString(room->getBedType()));
        thumbnail->setText("🛏");
        thumbnail->setStyleSheet("background-color: #E2E8F0; border-radius: 8px; font-size: 24px; color: #475569;");
        typeColor = QColor("#64748B");
    } else if (room->getRoomTypeName() == "Deluxe") {
        typeLabel = "Deluxe";
        specLabel = QString("%1m² · %2").arg(room->getArea()).arg(QString::fromStdString(room->getBedType()));
        thumbnail->setText("✨");
        thumbnail->setStyleSheet("background-color: #E0F2FE; border-radius: 8px; font-size: 24px; color: #0369A1;");
        typeColor = QColor("#0284C7");
    } else if (room->getRoomTypeName() == "Suite") {
        typeLabel = "Suite";
        specLabel = QString("%1m² · %2").arg(room->getArea()).arg(QString::fromStdString(room->getBedType()));
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
    const QDateTime statusNow = QDateTime::currentDateTime();
    const bool isCleaning = hasActiveCleaning(m_manager, room->getRoomNumber(), statusNow);
    const bool isUnderMaintenance = !room->getIsAvailable()
        || m_manager->isRoomBlockedAt(room->getRoomNumber(), QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toStdString());
    if (isUnderMaintenance) {
        statusBadge->setText(isCleaning ? "Cleaning" : "Maintenance");
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

    // Modified: Label the displayed rate as hourly so staff do not confuse the time-based bill with the retired nightly price.
    auto* priceLabel = new QLabel(formatMoney(room->getBasePrice()) + " VND / hour", card);
    priceLabel->setStyleSheet("font-size: 13px; font-weight: 800; color: #3B58FF;");

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
    m_lastRoomStateSignature = roomStateSignature();
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
        m_markRoomReadyBtn->setVisible(false);
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

    const QDateTime now = QDateTime::currentDateTime();
    const bool isCleaning = hasActiveCleaning(m_manager, room->getRoomNumber(), now);
    // Modified: Expose the early-release control only for an active Cleaning block; scheduled Maintenance must remain a separate workflow.
    m_markRoomReadyBtn->setVisible(isCleaning);
    const bool isUnderMaintenance = !room->getIsAvailable()
        || m_manager->isRoomBlockedAt(room->getRoomNumber(), QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toStdString());
    QString operationalDescription;
    if (isUnderMaintenance) {
        m_detailStatusLabel->setText(isCleaning ? "Cleaning" : "Maintenance");
        m_detailStatusLabel->setObjectName("detailBadgeMaint");
        operationalDescription = isCleaning
            ? "Cleaning is in progress after checkout. Mark the room ready only after turnover work is complete."
            : "This room is currently under scheduled maintenance. Please do not assign guests at this time.";
    } else if (isOccupied) {
        m_detailStatusLabel->setText("Occupied");
        m_detailStatusLabel->setObjectName("detailBadgeOccupied");
        operationalDescription = QString("Room is currently occupied (%1). Booking details are available in the reservation tab.").arg(QString::fromStdString(occupantName));
    } else if (awaitingBooking) {
        const auto customer = awaitingBooking->getCustomer();
        const QString guestName = customer ? QString::fromStdString(customer->getName()) : QStringLiteral("the booked guest");
        m_detailStatusLabel->setText("Awaiting check-in");
        m_detailStatusLabel->setObjectName("detailBadgeAwaiting");
        // Modified: Explain that an unarrived reservation still protects the room's inventory for its booked date range.
        operationalDescription = QString("Awaiting check-in for %1. The room remains reserved and cannot be assigned to another booking for this stay.").arg(guestName);
    } else {
        m_detailStatusLabel->setText("Available");
        m_detailStatusLabel->setObjectName("detailBadgeAvailable");
        operationalDescription = "This room is available for a booking during eligible dates.";
    }
    // Refresh label styles to apply name changes
    m_detailStatusLabel->style()->unpolish(m_detailStatusLabel);
    m_detailStatusLabel->style()->polish(m_detailStatusLabel);

    const std::string typeName = room->getRoomTypeName();
    m_detailSizeLabel->setText(QString("📏 %1m²").arg(room->getArea()));
    m_detailBedLabel->setText(QString("🛏 %1").arg(QString::fromStdString(room->getBedType())));
    m_detailGuestLabel->setText(QString("👤 %1 Guests").arg(room->getMaximumGuests()));
    // Modified: Keep the live operational explanation visible and append the static room description instead of overwriting the current room state.
    const QString roomDescription = QString::fromStdString(room->getDescription()).trimmed();
    m_detailDescLabel->setText(roomDescription.isEmpty()
        ? operationalDescription
        : operationalDescription + "\n\nRoom details: " + roomDescription);

    QStringList amenitiesList = QString::fromStdString(room->getAmenities()).split(", ", Qt::SkipEmptyParts);
    QString formattedAmenities = "<table width='100%' cellpadding='4'>";
    for (int i = 0; i < amenitiesList.size(); i += 2) {
        formattedAmenities += "<tr>";
        formattedAmenities += "<td width='50%'><span style='color: #3B58FF;'>✔</span> " + amenitiesList[i] + "</td>";
        if (i + 1 < amenitiesList.size()) {
            formattedAmenities += "<td width='50%'><span style='color: #3B58FF;'>✔</span> " + amenitiesList[i+1] + "</td>";
        } else {
            formattedAmenities += "<td width='50%'></td>";
        }
        formattedAmenities += "</tr>";
    }
    formattedAmenities += "</table>";
    m_detailAmenitiesLabel->setText(formattedAmenities);
    m_detailAmenitiesLabel->setTextFormat(Qt::RichText);

    if (typeName == "Standard") {
        // Modified: Present the Standard image collection in Room Management with the same carousel behavior used by Room Info.
        m_detailImageLabel->hide();
        m_detailRoomImageCarousel->setGallery("Standard", 6);
        m_detailRoomImageCarousel->show();
        m_detailExtraFeeLabel->setVisible(false);
    } else if (typeName == "Deluxe") {
        // Modified: Show the Deluxe gallery in the persistent room-details panel instead of a static placeholder.
        m_detailImageLabel->hide();
        m_detailRoomImageCarousel->setGallery("Deluxe", 8);
        m_detailRoomImageCarousel->show();
        m_detailExtraFeeLabel->setText(QString("💰 Mini Bar Fee: %1 VND").arg(formatMoney(room->getExtraFeeAmount())));
        m_detailExtraFeeLabel->setVisible(true);
    } else if (typeName == "Suite") {
        // Modified: Show the shared Suite gallery in Room Management so room details and booking review use the same images and navigation.
        m_detailImageLabel->hide();
        m_detailRoomImageCarousel->setGallery("Suite", 9);
        m_detailRoomImageCarousel->show();
        m_detailExtraFeeLabel->setText(QString("👑 Premium Service Fee: %1 VND").arg(formatMoney(room->getExtraFeeAmount())));
        m_detailExtraFeeLabel->setVisible(true);
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

    const QDateTime now = QDateTime::currentDateTime();
    for (const auto& booking : m_manager->getBookings()) {
        if (!booking || booking->isDeleted() || booking->isCancelled() || !booking->getRoom()) {
            continue;
        }
        QDateTime plannedStart = QDateTime::fromString(
            QString::fromStdString(booking->getPlannedCheckInAt()), Qt::ISODateWithMs);
        QDateTime plannedEnd = QDateTime::fromString(
            QString::fromStdString(booking->getPlannedCheckOutAt()), Qt::ISODateWithMs);
        if (!plannedStart.isValid()) {
            plannedStart = QDateTime(QDate::fromString(QString::fromStdString(booking->getCheckInDate()), Qt::ISODate), QTime(0, 0));
        }
        if (!plannedEnd.isValid()) {
            plannedEnd = QDateTime(QDate::fromString(QString::fromStdString(booking->getCheckOutDate()), Qt::ISODate), QTime(0, 0));
        }
        if (booking->getRoom()->getRoomNumber() == roomNumber
            && m_manager->getBookingState(*booking) == BookingState::UPCOMING
            && plannedStart.isValid() && plannedEnd.isValid() && plannedStart <= now && now < plannedEnd) {
            // Modified: Derive Awaiting from the scheduled timestamp interval rather than treating every date as a full-day stay.
            return booking;
        }
    }
    return nullptr;
}

QString RoomPageWidget::roomStateSignature() const
{
    if (!m_manager) return {};

    const QDateTime now = QDateTime::currentDateTime();
    std::unordered_map<std::string, QString> bookingStateByRoom;
    std::unordered_set<std::string> activeRooms;
    for (const auto& booking : m_manager->getBookings()) {
        if (!booking || booking->isCancelled() || booking->isDeleted() || !booking->getRoom()) continue;

        const std::string roomNumber = booking->getRoom()->getRoomNumber();
        const BookingState state = m_manager->getBookingState(*booking);
        if (state == BookingState::ACTIVE) {
            activeRooms.insert(roomNumber);
            bookingStateByRoom[roomNumber] = "occupied:" + QString::fromStdString(booking->getBookingId());
            continue;
        }
        if (state != BookingState::UPCOMING || activeRooms.find(roomNumber) != activeRooms.end()) continue;

        QDateTime plannedStart = QDateTime::fromString(
            QString::fromStdString(booking->getPlannedCheckInAt()), Qt::ISODateWithMs);
        QDateTime plannedEnd = QDateTime::fromString(
            QString::fromStdString(booking->getPlannedCheckOutAt()), Qt::ISODateWithMs);
        if (!plannedStart.isValid()) {
            plannedStart = QDateTime(
                QDate::fromString(QString::fromStdString(booking->getCheckInDate()), Qt::ISODate), QTime(0, 0));
        }
        if (!plannedEnd.isValid()) {
            plannedEnd = QDateTime(
                QDate::fromString(QString::fromStdString(booking->getCheckOutDate()), Qt::ISODate), QTime(0, 0));
        }
        if (plannedStart.isValid() && plannedEnd.isValid() && plannedStart <= now && now < plannedEnd) {
            bookingStateByRoom[roomNumber] = "awaiting:" + QString::fromStdString(booking->getBookingId());
        }
    }

    QStringList stateParts;
    stateParts.reserve(static_cast<int>(m_manager->getRooms().size()));
    for (const auto& room : m_manager->getRooms()) {
        if (!room || room->isArchived()) continue;

        QString state = "available";
        if (!room->getIsAvailable()
            || m_manager->isRoomBlockedAt(room->getRoomNumber(), now.toString(Qt::ISODateWithMs).toStdString())) {
            state = "maintenance";
        } else {
            const auto bookingState = bookingStateByRoom.find(room->getRoomNumber());
            if (bookingState != bookingStateByRoom.end()) state = bookingState->second;
        }
        stateParts.append(QString::fromStdString(room->getRoomNumber()) + '=' + state);
    }
    return stateParts.join('|');
}

void RoomPageWidget::onAddRoomClicked() {
    RoomDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        std::string roomNum = dialog.getRoomNumber().toStdString();
        double price = dialog.getBasePrice();
        RoomType type = dialog.getRoomType();
        double extraFee = dialog.getExtraFee();
        double area = dialog.getArea();
        std::string bedType = dialog.getBedType().toStdString();
        int maxGuests = dialog.getMaxGuests();
        std::string desc = dialog.getDescription().toStdString();
        std::string amen = dialog.getAmenities().toStdString();

        if (m_manager->roomNumberExists(roomNum)) {
            QMessageBox::warning(this, "Add room error", "The room number already exists in the system.");
            return;
        }

        std::string errMsg;
        // Modified: Set subtype fees through the room interface instead of concrete casts.
        if (m_manager->registerRoom(type, roomNum, price, area, bedType, maxGuests, desc, amen, errMsg)) {
            if (!m_manager->updateRoomDetails(roomNum, price, extraFee, area, bedType, maxGuests, desc, amen, errMsg)) {
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
                    maintenanceEndExclusive(dialog.getMaintenanceEndDate()).toStdString(),
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

    RoomDialog dialog(QString::fromStdString(roomNum), room->getBasePrice(), type, extraFee, room->getIsAvailable(), 
                      room->getArea(), QString::fromStdString(room->getBedType()), room->getMaximumGuests(), 
                      QString::fromStdString(room->getDescription()), QString::fromStdString(room->getAmenities()), this);
    std::vector<RoomMaintenance> existingSchedules;
    const std::string today = QDate::currentDate().toString(Qt::ISODate).toStdString();
    for (const RoomMaintenance& maintenance : m_manager->getRoomMaintenances()) {
        if (maintenance.isMaintenance() && maintenance.getRoomNumber() == roomNum && maintenance.getEndDate() > today) {
            existingSchedules.push_back(maintenance);
        }
    }
    dialog.setExistingMaintenanceSchedules(existingSchedules, m_manager->getMaintenanceGuestNotices());
    if (dialog.exec() == QDialog::Accepted) {
        double newPrice = dialog.getBasePrice();
        double newExtraFee = dialog.getExtraFee();
        const bool scheduleMaintenance = dialog.shouldScheduleMaintenance();

        std::string errorMessage;
        bool pendingRoomMutation = false;
        const auto restorePersistedRoomState = [this, &pendingRoomMutation]() {
            if (pendingRoomMutation) {
                // Modified: Rehydrate the saved snapshot when a later step fails so maintenance and room edits never remain changed only in memory.
                DataManager::getInstance().restoreLastSavedState(*m_manager);
                refreshData();
            }
        };
        if (!dialog.getMaintenanceIdToCancel().isEmpty() &&
            !m_manager->cancelRoomMaintenance(dialog.getMaintenanceIdToCancel().toStdString(), errorMessage)) {
            QMessageBox::warning(this, "Maintenance cancellation failed", QString::fromStdString(errorMessage));
            return;
        }
        pendingRoomMutation = !dialog.getMaintenanceIdToCancel().isEmpty();
        if (!dialog.getMaintenanceIdToConfirm().isEmpty() &&
            !m_manager->confirmRoomMaintenance(dialog.getMaintenanceIdToConfirm().toStdString(), errorMessage)) {
            restorePersistedRoomState();
            QMessageBox::warning(this, "Maintenance case still pending", QString::fromStdString(errorMessage));
            return;
        }
        pendingRoomMutation = pendingRoomMutation || !dialog.getMaintenanceIdToConfirm().isEmpty();
        if (scheduleMaintenance) {
            const QString maintenanceStart = dialog.getMaintenanceStartDate();
            const QString maintenanceEndAt = maintenanceEndExclusive(dialog.getMaintenanceEndDate());
            const auto impacts = m_manager->getMaintenanceImpactWarnings(
                roomNum, maintenanceStart.toStdString(), maintenanceEndAt.toStdString());
            if (!impacts.empty()) {
                QStringList impactLines;
                for (const std::string& impact : impacts) {
                    impactLines.append("• " + QString::fromStdString(impact));
                }
                // Modified: Show every affected stay before a Maintenance case creates a soft hold and simulated guest-contact notices.
                CustomConfirmDialog impactConfirmation(
                    "Maintenance affects reservations",
                    "What will happen: this schedule creates a temporary maintenance hold and logs simulated contact notices.\n\nAffected stays:\n"
                        + impactLines.join("\n")
                        + "\n\nNext step: review these stays, then create the hold only if the schedule is intended.",
                    true,
                    this,
                    "Create maintenance hold",
                    "Review schedule",
                    true);
                if (impactConfirmation.exec() != QDialog::Accepted || !impactConfirmation.isConfirmed()) {
                    return;
                }
            }
        }
        if (scheduleMaintenance && !m_manager->scheduleRoomMaintenance(roomNum,
                dialog.getMaintenanceStartDate().toStdString(),
                maintenanceEndExclusive(dialog.getMaintenanceEndDate()).toStdString(),
                dialog.getMaintenanceNote().toStdString(), errorMessage)) {
            restorePersistedRoomState();
            QMessageBox::warning(this, "Maintenance schedule conflict", QString::fromStdString(errorMessage));
            return;
        }
        pendingRoomMutation = pendingRoomMutation || scheduleMaintenance;
        const QString maintenanceWorkflowMessage = QString::fromStdString(errorMessage);
        if (!scheduleMaintenance && !room->getIsAvailable() &&
            !m_manager->setRoomAvailability(roomNum, true, errorMessage)) {
            restorePersistedRoomState();
            QMessageBox::warning(this, "Room status unavailable", QString::fromStdString(errorMessage));
            return;
        }
        pendingRoomMutation = pendingRoomMutation || (!scheduleMaintenance && !room->getIsAvailable());


        // Modified: Schedule maintenance by date range so future bookings remain valid outside the closure interval.
        double area = dialog.getArea();
        std::string bedType = dialog.getBedType().toStdString();
        int maxGuests = dialog.getMaxGuests();
        std::string desc = dialog.getDescription().toStdString();
        std::string amen = dialog.getAmenities().toStdString();

        if (!m_manager->updateRoomDetails(
                roomNum, newPrice, newExtraFee, area, bedType, maxGuests, desc, amen, errorMessage)) {
            restorePersistedRoomState();
            QMessageBox::warning(
                this,
                "Room update failed",
                QString::fromStdString(errorMessage));
            return;
        }
        pendingRoomMutation = true;

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

void RoomPageWidget::onMarkRoomReadyClicked()
{
    auto* item = m_roomListWidget->currentItem();
    if (!item || !m_manager) {
        return;
    }

    const std::string roomNumber = item->data(Qt::UserRole).toString().toStdString();
    CustomConfirmDialog confirmation(
        "Mark room ready",
        "Confirm that cleaning is complete and release this room for a check-in?",
        false,
        this);
    if (confirmation.exec() != QDialog::Accepted || !confirmation.isConfirmed()) {
        return;
    }

    std::string errorMessage;
    // Modified: An explicit staff action ends only the current Cleaning block; it never changes the scheduled Maintenance record.
    if (!m_manager->markRoomReady(roomNumber, "Staff", errorMessage)) {
        QMessageBox::warning(this, "Room not ready", QString::fromStdString(errorMessage));
        return;
    }
    if (!DataManager::getInstance().commitChanges(*m_manager)) {
        refreshData();
        QMessageBox::critical(this, "Save Room Failed", "The room-ready update was not saved. The previous database state has been restored.");
        return;
    }
    refreshData();
    CustomSuccessDialog("Room marked ready for check-in.", this).exec();
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
