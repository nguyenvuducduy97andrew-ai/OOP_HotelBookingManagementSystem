#include "RoomStatusPageWidget.h"
#include "StandardRoom.h"
#include "DeluxeRoom.h"
#include "SuiteRoom.h"
#include "Customer.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QDate>
#include <QDebug>
#include "CustomConfirmDialog.h"
#include <QMouseEvent>
#include <QMessageBox>
#include <QFrame>
#include <functional>

class RoomStatusCard : public QFrame {
private:
    std::shared_ptr<Room> m_room;
    std::function<void()> m_onDoubleClicked;

public:
    RoomStatusCard(std::shared_ptr<Room> room, std::function<void()> onDoubleClicked, QWidget* parent = nullptr)
        : QFrame(parent), m_room(room), m_onDoubleClicked(onDoubleClicked) {
        setCursor(Qt::PointingHandCursor);
    }

protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override {
        QFrame::mouseDoubleClickEvent(event);
        if (m_onDoubleClicked) {
            m_onDoubleClicked();
        }
    }
};

RoomStatusPageWidget::RoomStatusPageWidget(HotelManager* manager, QWidget *parent)
    : QWidget(parent), m_manager(manager) {
    setupUI();
    refreshData();
}

void setupRoomStatusPageStyle(QWidget* widget) {
    widget->setStyleSheet(R"(
        QLabel#pageTitle {
            font-size: 20px;
            font-weight: 800;
            color: #2B3674;
        }
        QLineEdit#searchEdit {
            background-color: #F4F7FE;
            border: 1px solid #E9EDF7;
            border-radius: 10px;
            padding: 8px 16px;
            font-size: 13px;
            color: #2B3674;
            min-width: 250px;
        }
        QLineEdit#searchEdit:focus {
            border: 1px solid #005BFE;
        }
        QComboBox.filterCombo {
            background-color: #F4F7FE;
            border: 1px solid #E9EDF7;
            border-radius: 10px;
            padding: 8px 16px;
            font-size: 13px;
            color: #2B3674;
            min-width: 150px;
        }
        QComboBox QAbstractItemView {
            background-color: #FFFFFF;
            color: #2B3674;
            selection-background-color: #005BFE;
            selection-color: #FFFFFF;
            border: 1px solid #E9EDF7;
        }
        QScrollArea {
            border: none;
            background-color: transparent;
        }
    )");
}

void RoomStatusPageWidget::setupUI() {
    setupRoomStatusPageStyle(this);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(16);

    // Header Title
    auto* pageTitle = new QLabel("Room Status Overview", this);
    pageTitle->setObjectName("pageTitle");
    mainLayout->addWidget(pageTitle);

    // Filters Row
    auto* filterRow = new QHBoxLayout();
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setObjectName("searchEdit");
    m_searchEdit->setPlaceholderText("Search room number...");

    m_typeCombo = new QComboBox(this);
    m_typeCombo->setObjectName("typeCombo");
    m_typeCombo->setProperty("class", "filterCombo");
    m_typeCombo->addItems({"All room types", "Standard", "Deluxe", "Suite"});

    m_statusCombo = new QComboBox(this);
    m_statusCombo->setObjectName("statusCombo");
    m_statusCombo->setProperty("class", "filterCombo");
    m_statusCombo->addItems({"All statuses", "Available", "Occupied", "Maintenance"});

    filterRow->addWidget(m_searchEdit);
    filterRow->addWidget(m_typeCombo);
    filterRow->addWidget(m_statusCombo);
    filterRow->addStretch();
    mainLayout->addLayout(filterRow);

    // Scroll Area for Grid
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    
    m_gridContainer = new QWidget(scroll);
    m_gridLayout = new QGridLayout(m_gridContainer);
    m_gridLayout->setSpacing(15);
    m_gridLayout->setContentsMargins(5, 5, 5, 5);
    
    m_gridContainer->setLayout(m_gridLayout);
    scroll->setWidget(m_gridContainer);
    mainLayout->addWidget(scroll);

    // Connects
    connect(m_searchEdit, &QLineEdit::textChanged, this, &RoomStatusPageWidget::onFiltersChanged);
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &RoomStatusPageWidget::onFiltersChanged);
    connect(m_statusCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &RoomStatusPageWidget::onFiltersChanged);
}

void RoomStatusPageWidget::onFiltersChanged() {
    refreshData();
}

QWidget* RoomStatusPageWidget::createRoomStatusCard(const std::shared_ptr<Room>& room) {
    auto* card = new RoomStatusCard(room, [=]() {
        bool isOccupied = false;
        if (m_manager) {
            for (const auto& booking : m_manager->getBookings()) {
                if (booking && !booking->isCancelled() && !booking->isDeleted() && booking->getRoom() &&
                    booking->getRoom()->getRoomNumber() == room->getRoomNumber()) {
                    if (m_manager->getBookingState(*booking) == BookingState::ACTIVE) {
                        isOccupied = true;
                        break;
                    }
                }
            }
        }

        if (isOccupied) {
            QMessageBox::warning(this, "Cannot change status", 
                QString("Room %1 is currently occupied and cannot be moved into maintenance.").arg(QString::fromStdString(room->getRoomNumber())));
            return;
        }

        if (room->getIsAvailable()) {
            CustomConfirmDialog dialog("Confirm maintenance", QString("Do you want to move room %1 into maintenance status?").arg(QString::fromStdString(room->getRoomNumber())), false, this);
            if (dialog.exec() == QDialog::Accepted && dialog.isConfirmed()) {
                room->setIsAvailable(false);
                refreshData();
            }
        } else {
            CustomConfirmDialog dialog("Confirm return to service", QString("Complete maintenance for room %1 and return it to service?").arg(QString::fromStdString(room->getRoomNumber())), false, this);
            if (dialog.exec() == QDialog::Accepted && dialog.isConfirmed()) {
                room->setIsAvailable(true);
                refreshData();
            }
        }
    });
    
    // Check occupied status
    bool isOccupied = false;
    std::string occupantName = "";
    if (m_manager) {
        for (const auto& booking : m_manager->getBookings()) {
            if (booking && !booking->isCancelled() && !booking->isDeleted() && booking->getRoom() &&
                booking->getRoom()->getRoomNumber() == room->getRoomNumber()) {
                if (m_manager->getBookingState(*booking) == BookingState::ACTIVE) {
                    isOccupied = true;
                    if (booking->getCustomer()) {
                        occupantName = booking->getCustomer()->getName();
                    }
                    break;
                }
            }
        }
    }

    QString statusText = "AVAILABLE";
    QString typeLabel = "Standard";
    QString extraText = "Ready for guests";

    QString cardStyle = "border-radius: 12px; border: 1px solid #E2E8F0;";
    QString titleStyle = "font-size: 15px; font-weight: 800; color: #2B3674;";
    QString typeStyle = "font-size: 10px; font-weight: 700; padding: 2px 6px; border-radius: 4px;";
    QString descStyle = "font-size: 11px; color: #A3AED0;";

    // Dynamic coloring based on status
    if (!room->getIsAvailable()) {
        statusText = "MAINTENANCE";
        extraText = "Under technical maintenance";
        cardStyle += "background-color: #FEF2F2; border: 1px solid #FCA5A5;";
        typeStyle += "background-color: #EF4444; color: white;";
    } else if (isOccupied) {
        statusText = "OCCUPIED";
        extraText = QString::fromStdString(occupantName);
        cardStyle += "background-color: #FFFBEB; border: 1px solid #FDE68A;";
        typeStyle += "background-color: #D97706; color: white;";
    } else {
        statusText = "AVAILABLE";
        cardStyle += "background-color: #ECFDF5; border: 1px solid #A7F3D0;";
        typeStyle += "background-color: #05CD99; color: white;";
    }

    card->setStyleSheet("QWidget { " + cardStyle + " }");

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(6);

    auto* row1 = new QHBoxLayout();
    auto* title = new QLabel("Room " + QString::fromStdString(room->getRoomNumber()), card);
    title->setStyleSheet(titleStyle);
    
    if (dynamic_cast<DeluxeRoom*>(room.get())) typeLabel = "Deluxe";
    else if (dynamic_cast<SuiteRoom*>(room.get())) typeLabel = "Suite";

    auto* badge = new QLabel(typeLabel, card);
    badge->setStyleSheet(typeStyle);
    
    row1->addWidget(title);
    row1->addStretch();
    row1->addWidget(badge);
    layout->addLayout(row1);

    auto* status = new QLabel(statusText, card);
    status->setStyleSheet("font-size: 11px; font-weight: 800; color: #2B3674;");
    layout->addWidget(status);

    auto* extra = new QLabel(extraText, card);
    extra->setStyleSheet(descStyle);
    layout->addWidget(extra);

    return card;
}

void RoomStatusPageWidget::refreshData() {
    // Clear old items in grid
    QLayoutItem* child;
    while ((child = m_gridLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    if (!m_manager) return;

    QString search = m_searchEdit->text().trimmed();
    int typeFilter = m_typeCombo->currentIndex(); // 0: All, 1: Standard, 2: Deluxe, 3: Suite
    int statusFilter = m_statusCombo->currentIndex(); // 0: All, 1: Available, 2: Occupied, 3: Maintenance

    int row = 0;
    int col = 0;
    const int colsCount = 4; // 4 cards per row

    for (const auto& room : m_manager->getRooms()) {
        if (!room) continue;

        // Type Filter check
        QString typeStr = "Standard";
        if (dynamic_cast<DeluxeRoom*>(room.get())) typeStr = "Deluxe";
        else if (dynamic_cast<SuiteRoom*>(room.get())) typeStr = "Suite";

        if (typeFilter > 0) {
            if (typeFilter == 1 && typeStr != "Standard") continue;
            if (typeFilter == 2 && typeStr != "Deluxe") continue;
            if (typeFilter == 3 && typeStr != "Suite") continue;
        }

        // Status Filter check
        bool isOccupied = false;
        for (const auto& booking : m_manager->getBookings()) {
            if (booking && !booking->isCancelled() && !booking->isDeleted() && booking->getRoom() &&
                booking->getRoom()->getRoomNumber() == room->getRoomNumber()) {
                if (m_manager->getBookingState(*booking) == BookingState::ACTIVE) {
                    isOccupied = true;
                    break;
                }
            }
        }

        if (statusFilter > 0) {
            if (statusFilter == 1 && (!room->getIsAvailable() || isOccupied)) continue; // Available
            if (statusFilter == 2 && (!room->getIsAvailable() || !isOccupied)) continue; // Occupied
            if (statusFilter == 3 && room->getIsAvailable()) continue; // Maintenance
        }

        // Search filter check
        QString roomNum = QString::fromStdString(room->getRoomNumber());
        if (!search.isEmpty() && !roomNum.contains(search)) {
            continue;
        }

        auto* card = createRoomStatusCard(room);
        m_gridLayout->addWidget(card, row, col);

        col++;
        if (col >= colsCount) {
            col = 0;
            row++;
        }
    }
}
