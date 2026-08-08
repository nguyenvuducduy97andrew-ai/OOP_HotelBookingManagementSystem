#include "RoomStatusPageWidget.h"
#include "ui_RoomStatusPageWidget.h"
#include "Customer.h"
#include "RoomManager.h"
#include "RoomInfoDialog.h"
#include "SchedulePickerDialog.h"
#include "SearchFieldUi.h"
#include <QGridLayout>
#include <QDate>
#include <QDateTime>
#include <QTime>
#include <QPushButton>
#include <QLabel>
#include <QSizePolicy>
#include <QFrame>
#include <QComboBox>
#include <QMap>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace {
QString displayRoomCardMoment(const std::string& timestamp, const std::string& legacyDate)
{
    QDateTime value = QDateTime::fromString(QString::fromStdString(timestamp), Qt::ISODateWithMs);
    if (!value.isValid()) {
        value = QDateTime::fromString(QString::fromStdString(timestamp), Qt::ISODate);
    }
    if (value.isValid()) {
        return value.toString("dd/MM HH:mm");
    }
    const QDate date = QDate::fromString(QString::fromStdString(legacyDate), Qt::ISODate);
    return date.isValid() ? date.toString("dd/MM") : QString::fromStdString(legacyDate);
}

int floorForRoomNumber(const QString& roomNumber)
{
    QString digits;
    for (const QChar character : roomNumber.trimmed()) {
        if (!character.isDigit()) break;
        digits.append(character);
    }

    bool ok = false;
    const int numericRoom = digits.toInt(&ok);
    if (!ok || numericRoom < 0) return 0;
    // Conventional room numbering keeps the final two digits for the room on a floor.
    return digits.size() >= 3 ? numericRoom / 100 : numericRoom / 10;
}
}

RoomStatusPageWidget::RoomStatusPageWidget(HotelManager* manager, QWidget *parent)
    : QWidget(parent), ui(new Ui::RoomStatusPageWidget), m_manager(manager) {
    ui->setupUi(this);
    
    // Prevent the search bar from receiving focus automatically.
    this->setFocusPolicy(Qt::StrongFocus);
    this->setFocus();

    // Modified: Start with one adult so the first valid schedule can immediately filter rooms and open the booking flow.
    ui->lblAdultCount->setText("1");
    ui->lblChildrenCount->setText("0");

    const QDateTime now = QDateTime::currentDateTime();
    m_selectedCheckIn = QDateTime(now.date(), QTime(now.time().hour(), 0)).addSecs(60 * 60);
    m_selectedCheckOut = m_selectedCheckIn.addSecs(60 * 60);
    ui->dateEditCheckIn->setText(m_selectedCheckIn.date().toString("dd/MM/yyyy"));
    ui->dateEditCheckOut->setText(m_selectedCheckOut.date().toString("dd/MM/yyyy"));

    auto* filterLayout = qobject_cast<QGridLayout*>(ui->frameFilterPanel->layout());
    if (filterLayout) {
        ui->lblCheckOutTitle->hide();
        ui->dateEditCheckIn->hide();
        ui->dateEditCheckOut->hide();
        m_scheduleButton = new QPushButton("Schedule booking", ui->frameFilterPanel);
        m_scheduleButton->setObjectName("btnChooseSchedule");
        // Modified: Give the shared schedule entry a dedicated visible treatment instead of inheriting an empty legacy date-field cell.
        m_scheduleButton->setMinimumHeight(42);
        m_scheduleButton->setMinimumWidth(150);
        m_scheduleButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_scheduleButton->setStyleSheet(
            "QPushButton { background: #EFF6FF; color: #1D4ED8; border:1px solid #BFDBFE; "
            "border-radius:10px; padding:9px 14px; font-weight:700; text-align:left; }"
            "QPushButton:hover { background: #DBEAFE; border-color: #60A5FA; }");
        const QString fieldStyle =
            "QPushButton { background:#FFFFFF; color:#2B3674; border:1px solid #D8E2F0; "
            "border-radius:10px; padding:9px 12px; font-weight:600; text-align:left; }"
            "QPushButton:hover { background: #F8FAFF; border-color: #BFDBFE; }"
            "QPushButton:focus { border:1px solid #93C5FD; }";
        m_checkInScheduleField = new QPushButton(ui->frameFilterPanel);
        m_checkOutScheduleField = new QPushButton(ui->frameFilterPanel);
        m_checkInScheduleField->setMinimumHeight(42);
        m_checkOutScheduleField->setMinimumHeight(42);
        m_checkInScheduleField->setMinimumWidth(190);
        m_checkOutScheduleField->setMinimumWidth(190);
        m_checkInScheduleField->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_checkOutScheduleField->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_checkInScheduleField->setStyleSheet(fieldStyle);
        m_checkOutScheduleField->setStyleSheet(fieldStyle);

        m_roomTypeCombo = new QComboBox(ui->frameFilterPanel);
        m_roomTypeCombo->setObjectName("roomTypeCombo");
        m_roomTypeCombo->addItem("All room types", "All");
        m_roomTypeCombo->addItem("Standard", "Standard");
        m_roomTypeCombo->addItem("Deluxe", "Deluxe");
        m_roomTypeCombo->addItem("Suite", "Suite");
        m_roomTypeCombo->setMinimumHeight(42);
        m_roomTypeCombo->setMinimumWidth(155);
        m_roomTypeCombo->setStyleSheet(
            "QComboBox { background:#FFFFFF; color:#2B3674; border:1px solid #D8E2F0; "
            "border-radius:10px; padding:9px 12px; font-weight:700; }"
            "QComboBox:hover { background:#F8FAFF; border-color:#BFDBFE; }");

        filterLayout->addWidget(m_scheduleButton, 1, 0, 1, 2);
        filterLayout->addWidget(m_checkInScheduleField, 1, 2, 1, 3);
        filterLayout->addWidget(m_checkOutScheduleField, 1, 5, 1, 3);
        filterLayout->addWidget(m_roomTypeCombo, 1, 8, 1, 3);
        updateScheduleFields();

        connect(m_scheduleButton, &QPushButton::clicked, this, [this]()
                { openSchedulePicker(false); });
        connect(m_checkInScheduleField, &QPushButton::clicked, this, [this]()
                { openSchedulePicker(false); });
        connect(m_checkOutScheduleField, &QPushButton::clicked, this, [this]()
                { openSchedulePicker(true); });
        connect(
            m_roomTypeCombo,
            &QComboBox::currentIndexChanged,
            this,
            [this](int)
            {
                applyFilters();
            });
    }


    connect(ui->btnAddAdult, &QPushButton::clicked, this, [this]() {
        int val = ui->lblAdultCount->text().toInt();
        ui->lblAdultCount->setText(QString::number(val + 1));
        setAvailabilityMode(true);
    });
    connect(ui->btnMinusAdult, &QPushButton::clicked, this, [this]() {
        int val = ui->lblAdultCount->text().toInt();
        if (val > 1) {
            ui->lblAdultCount->setText(QString::number(val - 1));
            setAvailabilityMode(true);
        }
    });

    connect(ui->btnAddChild, &QPushButton::clicked, this, [this]() {
        int val = ui->lblChildrenCount->text().toInt();
        ui->lblChildrenCount->setText(QString::number(val + 1));
        setAvailabilityMode(true);
    });
    connect(ui->btnMinusChild, &QPushButton::clicked, this, [this]() {
        int val = ui->lblChildrenCount->text().toInt();
        if (val > 0) {
            ui->lblChildrenCount->setText(QString::number(val - 1));
            setAvailabilityMode(true);
        }
    });
    ui->btnFilterAll->hide();
    ui->btnFilterStandard->hide();
    ui->btnFilterDeluxe->hide();
    ui->btnFilterSuite->hide();


    // Debounce typing so a quick search updates the cards once instead of rebuilding on every keystroke.
    m_searchDebounceTimer = new QTimer(this);
    m_searchDebounceTimer->setSingleShot(true);
    m_searchDebounceTimer->setInterval(120);
    connect(m_searchDebounceTimer, &QTimer::timeout, this, &RoomStatusPageWidget::applyFilters);
    connect(ui->txtSearchRoom, &QLineEdit::textChanged, this, [this]() {
        m_searchDebounceTimer->start();
    });
    addSearchIcon(ui->txtSearchRoom);
    // Keep the original horizontal span while reducing the search bar's vertical footprint.
    ui->txtSearchRoom->setFixedHeight(32);

    // Keep button sizes fixed so the layout does not shift with shorter labels.
    ui->btnCheckAvailability->setMinimumWidth(190);
    ui->btnCheckAvailability->setMaximumWidth(220);
    ui->btnCheckAvailability->setMinimumHeight(42);

    // Availability is applied automatically; this action only restores the neutral filter state.
    ui->btnCheckAvailability->setText("Clear filters");
    connect(ui->btnCheckAvailability, &QPushButton::clicked,
            this, &RoomStatusPageWidget::clearFilters);

    ui->btnFilterAll->setChecked(true);

    QGridLayout* gridLayout = qobject_cast<QGridLayout*>(ui->scrollAreaWidgetContents->layout());
    if (!gridLayout) {
        gridLayout = new QGridLayout(ui->scrollAreaWidgetContents);
        ui->scrollAreaWidgetContents->setLayout(gridLayout);
        gridLayout->setContentsMargins(10, 20, 10, 10); 
        gridLayout->setSpacing(10);
        gridLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    }

    refreshData();
    m_statusRefreshTimer = new QTimer(this);
    m_statusRefreshTimer->setInterval(5 * 1000);
    // Modified: Refresh operational states promptly after a timed Cleaning block ends without polling while the page is hidden.
    connect(m_statusRefreshTimer, &QTimer::timeout, this, [this]() {
        if (isVisible() && roomStateSignature() != m_lastRoomStateSignature) {
            refreshData();
        }
    });
    m_statusRefreshTimer->start();
}

void RoomStatusPageWidget::updateScheduleFields()
{
    if (m_checkInScheduleField) {
        m_checkInScheduleField->setText("Check-in:   " + m_selectedCheckIn.toString("dd MMM yyyy, HH:mm"));
    }
    if (m_checkOutScheduleField) {
        m_checkOutScheduleField->setText("Check-out:   " + m_selectedCheckOut.toString("dd MMM yyyy, HH:mm"));
    }
}

void RoomStatusPageWidget::openSchedulePicker(bool startInCheckOutMode)
{
    const auto availabilityPredicate = [this](const QDateTime& start, const QDateTime& end) {
        if (!m_manager) {
            return false;
        }
        std::string availabilityError;
        const auto availableRooms = m_manager->getAvailableRoomsForPeriod(
            start.toString(Qt::ISODate).toStdString(), end.toString(Qt::ISODate).toStdString(),
            availabilityError);
        const int guests = ui->lblAdultCount->text().toInt() + ui->lblChildrenCount->text().toInt();
        return std::any_of(availableRooms.begin(), availableRooms.end(), [guests](const std::shared_ptr<Room>& room) {
            return room && (guests <= 0 || guests <= room->getMaximumGuests());
        });
    };

    // Both entry fields edit the same pair. Opening Check-out merely selects that mode first.
    SchedulePickerDialog picker(m_selectedCheckIn, m_selectedCheckOut, availabilityPredicate,
                                this, false, startInCheckOutMode);
    if (picker.exec() != QDialog::Accepted) {
        return;
    }

    m_selectedCheckIn = picker.selectedCheckIn();
    m_selectedCheckOut = picker.selectedCheckOut();
    updateScheduleFields();
    setAvailabilityMode(true);
}

void RoomStatusPageWidget::setupUI() {
    // Currently merged into constructor
}

void RoomStatusPageWidget::refreshData() {
    if (!m_manager) return;

    QGridLayout* gridLayout = qobject_cast<QGridLayout*>(ui->scrollAreaWidgetContents->layout());
    if (!gridLayout) return;

    m_availabilityCacheValid = false;
    clearFloorSections();
    // Clear old cards
    for (RoomCard* card : m_roomCards) {
        gridLayout->removeWidget(card);
        delete card;
    }
    m_roomCards.clear();

    std::unordered_map<std::string, std::shared_ptr<Booking>> activeBookingsByRoom;
    std::unordered_map<std::string, std::shared_ptr<Booking>> awaitingBookingsByRoom;
    const QDateTime now = QDateTime::currentDateTime();
    const std::string today = now.date().toString(Qt::ISODate).toStdString();
    for (const auto& booking : m_manager->getBookings()) {
        if (!booking || booking->isCancelled() || booking->isDeleted()) {
            continue;
        }

        const auto bookedRoom = booking->getRoom();
        if (!bookedRoom) {
            continue;
        }

        const BookingState state = m_manager->getBookingState(*booking);
        if (state == BookingState::ACTIVE) {
            activeBookingsByRoom.emplace(bookedRoom->getRoomNumber(), booking);
        } else if (state == BookingState::UPCOMING) {
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
            if (plannedStart.isValid() && plannedEnd.isValid() && plannedStart <= now && now < plannedEnd) {
                // Modified: Awaiting means the guest has a current scheduled stay but staff has not performed actual check-in.
                awaitingBookingsByRoom.emplace(bookedRoom->getRoomNumber(), booking);
            }
        }
    }

    for (const auto& room : m_manager->getRooms()) {
        if (!room || room->isArchived()) continue;

        RoomCard* card = new RoomCard(ui->scrollAreaWidgetContents);
        card->setRoomNumber(QString::fromStdString(room->getRoomNumber()));

        // Modified: Render room cards from the virtual room type name.
        QString roomType = QString::fromStdString(room->getRoomTypeName());
        card->setRoomType(roomType);

        const auto activeBookingIt = activeBookingsByRoom.find(room->getRoomNumber());
        const bool isOccupied = activeBookingIt != activeBookingsByRoom.end();
        const std::shared_ptr<Booking> activeBooking = isOccupied ? activeBookingIt->second : nullptr;
        const auto awaitingBookingIt = awaitingBookingsByRoom.find(room->getRoomNumber());
        const std::shared_ptr<Booking> awaitingBooking = awaitingBookingIt != awaitingBookingsByRoom.end()
            ? awaitingBookingIt->second : nullptr;

        const bool isUnderMaintenance = !room->getIsAvailable()
            || m_manager->isRoomBlockedAt(room->getRoomNumber(), now.toString(Qt::ISODateWithMs).toStdString());
        if (isUnderMaintenance) {
            card->setMaintenance();
        } else if (isOccupied && activeBooking) {
            QString guestName = activeBooking->getCustomer() ? QString::fromStdString(activeBooking->getCustomer()->getName()) : "Unknown";
            QString phone = activeBooking->getCustomer() ? QString::fromStdString(activeBooking->getCustomer()->getPhoneNumber()) : "";
            QString idNumber = activeBooking->getCustomer() ? QString::fromStdString(activeBooking->getCustomer()->getDocumentNumber()) : "";
            
            // Modified: Include the operational time on room cards so reception can distinguish hourly stays on the same date.
            const QString dateIn = displayRoomCardMoment(
                activeBooking->getActualCheckInAt().empty() ? activeBooking->getPlannedCheckInAt() : activeBooking->getActualCheckInAt(),
                activeBooking->getActualCheckInAt().empty() ? activeBooking->getCheckInDate() : activeBooking->getActualCheckInDate());
            const QString dateOut = displayRoomCardMoment(activeBooking->getPlannedCheckOutAt(), activeBooking->getCheckOutDate());
            
            card->setOccupied(guestName, idNumber, phone, dateIn, dateOut);
        } else if (awaitingBooking) {
            const auto customer = awaitingBooking->getCustomer();
            const QString guestName = customer ? QString::fromStdString(customer->getName()) : "Awaiting check-in";
            const QString dateIn = displayRoomCardMoment(awaitingBooking->getPlannedCheckInAt(), awaitingBooking->getCheckInDate());
            const QString dateOut = displayRoomCardMoment(awaitingBooking->getPlannedCheckOutAt(), awaitingBooking->getCheckOutDate());
            card->setAwaiting(guestName, dateIn, dateOut);
        } else {
            // Modified: Available now means no active stay or arrival-due reservation holds the room today.
            card->setAvailable();
        }

        m_roomCards.append(card);

        connect(card, &RoomCard::cardClicked, this, [this, room, card]() {
            if (/*!m_isCheckAvailMode ||*/ !m_selectedCheckIn.isValid() || !m_selectedCheckOut.isValid()) {
                return;
            }

            const int reqAdults = ui->lblAdultCount->text().toInt();
            const int reqChildren = ui->lblChildrenCount->text().toInt();
            std::string availabilityError;
            const auto availableRooms = m_manager->getAvailableRoomsForPeriod(
                m_selectedCheckIn.toString(Qt::ISODate).toStdString(),
                m_selectedCheckOut.toString(Qt::ISODate).toStdString(), availabilityError);
            const bool stillAvailable = std::any_of(availableRooms.cbegin(), availableRooms.cend(),
                [&room, reqAdults, reqChildren](const std::shared_ptr<Room>& availableRoom) {
                    return availableRoom && availableRoom->getRoomNumber() == room->getRoomNumber()
                        && reqAdults > 0 && reqChildren >= 0
                        && reqAdults + reqChildren <= availableRoom->getMaximumGuests();
                });
            if (stillAvailable) {
                // The combined dialog opens on Room Info and expands in place when Booking is chosen.
                emit bookingRequested(QString::fromStdString(room->getRoomNumber()), m_selectedCheckIn, m_selectedCheckOut, reqAdults, reqChildren);
            }
        });

    }

    rebuildFloorSections();
    // Modified: Index active bookings once so rendering room cards no longer scans every booking for every room.
    applyFilters();
    m_lastRoomStateSignature = roomStateSignature();
}

void RoomStatusPageWidget::setFilterType(QString type) {
    if (m_roomTypeCombo) {
        const int index = m_roomTypeCombo->findData(type);
        if (index >= 0 && index != m_roomTypeCombo->currentIndex()) {
            const QSignalBlocker blocker(m_roomTypeCombo);
            m_roomTypeCombo->setCurrentIndex(index);
        }
    }
    applyFilters();
}

void RoomStatusPageWidget::setAvailabilityMode(bool enabled)
{
    m_isCheckAvailMode = enabled;
    if (m_isCheckAvailMode) {
        ui->btnCheckAvailability->setText("Clear filters");
        ui->btnCheckAvailability->setStyleSheet(
            "QPushButton { background-color: #E53935; color: white; border-radius: 10px; padding: 10px; font-weight: bold; }"
            "QPushButton:hover { background-color: #C62828; }"
        );
    } else {
        ui->btnCheckAvailability->setText("Clear filters");
        ui->btnCheckAvailability->setStyleSheet("");
    }
    applyFilters();
}

void RoomStatusPageWidget::clearFilters()
{
    const QSignalBlocker searchBlocker(ui->txtSearchRoom);
    ui->txtSearchRoom->clear();

    if (m_roomTypeCombo) {
        const QSignalBlocker typeBlocker(m_roomTypeCombo);
        const int allIndex = m_roomTypeCombo->findData("All");
        if (allIndex >= 0) m_roomTypeCombo->setCurrentIndex(allIndex);
    }

    ui->lblAdultCount->setText("1");
    ui->lblChildrenCount->setText("0");
    const QDateTime now = QDateTime::currentDateTime();
    m_selectedCheckIn = QDateTime(now.date(), QTime(now.time().hour(), 0)).addSecs(60 * 60);
    m_selectedCheckOut = m_selectedCheckIn.addSecs(60 * 60);
    updateScheduleFields();

    m_availabilityCacheValid = false;
    setAvailabilityMode(false);
}

void RoomStatusPageWidget::applyFilters() {
    const QString type = m_roomTypeCombo ? m_roomTypeCombo->currentData().toString() : QString("All");

    QString searchText = ui->txtSearchRoom->text().trimmed().toLower();
    QGridLayout* gridLayout = qobject_cast<QGridLayout*>(ui->scrollAreaWidgetContents->layout());
    if (!gridLayout) return;

    // Keep the persistent floor widgets and only update card membership/visibility.
    ui->scrollAreaWidgetContents->setUpdatesEnabled(false);

    const QDateTime reqIn = m_selectedCheckIn;
    const QDateTime reqOut = m_selectedCheckOut;
    int reqAdults = ui->lblAdultCount->text().toInt();
    int reqChildren = ui->lblChildrenCount->text().toInt();

    if (m_isCheckAvailMode) {
        if (!m_availabilityCacheValid
            || m_cachedAvailabilityCheckIn != reqIn
            || m_cachedAvailabilityCheckOut != reqOut) {
            m_cachedAvailableRoomNumbers.clear();
            std::string availabilityError;
            const auto availableRooms = m_manager->getAvailableRoomsForPeriod(
                reqIn.toString(Qt::ISODate).toStdString(),
                reqOut.toString(Qt::ISODate).toStdString(), availabilityError);
            for (const auto& room : availableRooms) {
                if (room) m_cachedAvailableRoomNumbers.insert(room->getRoomNumber());
            }
            m_cachedAvailabilityCheckIn = reqIn;
            m_cachedAvailabilityCheckOut = reqOut;
            m_availabilityCacheValid = true;
        }
    }

    for (auto* layout : m_floorLayoutByNumber) {
        QLayoutItem* item = nullptr;
        while ((item = layout->takeAt(0)) != nullptr) delete item;
    }

    const int columns = floorColumnCount();
    m_lastFloorColumnCount = columns;
    for (auto floorIt = m_cardsByFloor.cbegin(); floorIt != m_cardsByFloor.cend(); ++floorIt) {
        QGridLayout* roomsLayout = m_floorLayoutByNumber.value(floorIt.key(), nullptr);
        QWidget* section = m_floorSectionByNumber.value(floorIt.key(), nullptr);
        if (!roomsLayout || !section) continue;

        int visibleIndex = 0;
        for (RoomCard* card : floorIt.value()) {
            const bool matchType = type == "All" || card->getRoomType() == type;
            const bool matchSearch = searchText.isEmpty()
                || card->getGuestName().toLower().contains(searchText)
                || card->getRoomNumber().toLower().contains(searchText)
                || card->getRoomType().toLower().contains(searchText)
                || card->getDateIn().toLower().contains(searchText);

            bool matchAvail = true;
            if (m_isCheckAvailMode) {
                matchAvail = m_cachedAvailableRoomNumbers.find(card->getRoomNumber().toStdString())
                    != m_cachedAvailableRoomNumbers.end();
                if (matchAvail) {
                    const auto room = m_manager->findRoomByNumber(card->getRoomNumber().toStdString());
                    matchAvail = room && reqAdults > 0 && reqChildren >= 0
                        && reqAdults + reqChildren <= room->getMaximumGuests();
                }
            }

            card->setTempAvailMode(m_isCheckAvailMode && matchAvail);
            const bool visible = matchType && matchSearch && matchAvail;
            card->setVisible(visible);
            if (visible) {
                roomsLayout->addWidget(card, visibleIndex / columns, visibleIndex % columns,
                                       Qt::AlignTop | Qt::AlignLeft);
                ++visibleIndex;
            }
        }
        section->setVisible(visibleIndex > 0);
    }

    ui->scrollAreaWidgetContents->setUpdatesEnabled(true);
    ui->scrollAreaWidgetContents->update();
}

void RoomStatusPageWidget::rebuildFloorSections()
{
    auto* gridLayout = qobject_cast<QGridLayout*>(ui->scrollAreaWidgetContents->layout());
    if (!gridLayout) return;

    m_cardsByFloor.clear();
    for (RoomCard* card : m_roomCards) {
        if (card) m_cardsByFloor[floorForRoomNumber(card->getRoomNumber())].append(card);
    }

    int sectionRow = 0;
    for (auto floorIt = m_cardsByFloor.begin(); floorIt != m_cardsByFloor.end(); ++floorIt) {
        std::sort(floorIt.value().begin(), floorIt.value().end(), [](RoomCard* left, RoomCard* right) {
            bool leftOk = false;
            bool rightOk = false;
            const int leftNumber = left->getRoomNumber().toInt(&leftOk);
            const int rightNumber = right->getRoomNumber().toInt(&rightOk);
            return leftOk && rightOk ? leftNumber < rightNumber
                                     : left->getRoomNumber() < right->getRoomNumber();
        });

        auto* section = new QFrame(ui->scrollAreaWidgetContents);
        section->setObjectName("floorSection");
        section->setStyleSheet(
            "QFrame#floorSection { background:transparent; border:none; }"
            "QLabel#floorLabel { color: #64748B; font-size:16px; font-weight:600; padding:0px; }");
        auto* sectionLayout = new QVBoxLayout(section);
        sectionLayout->setContentsMargins(0, 0, 0, 20);
        sectionLayout->setSpacing(8);

        auto* floorLabel = new QLabel(
            floorIt.key() > 0 ? QString("Floor %1").arg(floorIt.key()) : QString("Other"), section);
        floorLabel->setObjectName("floorLabel");
        floorLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        floorLabel->setFixedWidth(86);
        floorLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        
        auto* hLine = new QFrame(section);
        hLine->setObjectName("floorLine");
        hLine->setFrameShape(QFrame::HLine);
        hLine->setFrameShadow(QFrame::Plain);
        hLine->setFixedHeight(1);
        hLine->setStyleSheet("background-color: #CBD5E1;");
        sectionLayout->addWidget(floorLabel);
        sectionLayout->addWidget(hLine);

        auto* roomsWidget = new QWidget(section);
        auto* roomsLayout = new QGridLayout(roomsWidget);
        roomsLayout->setContentsMargins(0, 0, 0, 0);
        roomsLayout->setHorizontalSpacing(10);
        roomsLayout->setVerticalSpacing(10);
        roomsLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        sectionLayout->addWidget(roomsWidget, 1);
        gridLayout->addWidget(section, sectionRow++, 0);

        m_floorSections.append(section);
        m_floorSectionByNumber.insert(floorIt.key(), section);
        m_floorLayoutByNumber.insert(floorIt.key(), roomsLayout);
    }
    gridLayout->setColumnStretch(0, 1);
}

int RoomStatusPageWidget::floorColumnCount() const
{
    const int viewportWidth = ui->scrollArea->viewport()->width();
    constexpr int floorLabelAndSpacing = 124;
    constexpr int cardWidthAndSpacing = 225;
    return std::max(1, (viewportWidth - floorLabelAndSpacing) / cardWidthAndSpacing);
}

void RoomStatusPageWidget::clearFloorSections()
{
    for (RoomCard* card : m_roomCards) {
        if (card) card->setParent(ui->scrollAreaWidgetContents);
    }
    for (QWidget* section : m_floorSections) {
        delete section;
    }
    m_floorSections.clear();
    m_cardsByFloor.clear();
    m_floorSectionByNumber.clear();
    m_floorLayoutByNumber.clear();

    if (auto* gridLayout = qobject_cast<QGridLayout*>(ui->scrollAreaWidgetContents->layout())) {
        QLayoutItem* item = nullptr;
        while ((item = gridLayout->takeAt(0)) != nullptr) delete item;
    }
}

void RoomStatusPageWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    const int columns = floorColumnCount();
    if (!m_roomCards.isEmpty() && columns != m_lastFloorColumnCount && !m_reflowPending) {
        m_reflowPending = true;
        QTimer::singleShot(0, this, [this]() {
            m_reflowPending = false;
            if (floorColumnCount() != m_lastFloorColumnCount) applyFilters();
        });
    }
}

QString RoomStatusPageWidget::roomStateSignature() const
{
    if (!m_manager) return {};

    const QDateTime now = QDateTime::currentDateTime();
    std::unordered_map<std::string, QString> bookingStateByRoom;
    std::unordered_set<std::string> activeRooms;
    for (const auto& booking : m_manager->getBookings()) {
        if (!booking || booking->isCancelled() || booking->isDeleted() || !booking->getRoom()) continue;

        const std::string roomNumber = booking->getRoom()->getRoomNumber();
        const BookingState bookingState = m_manager->getBookingState(*booking);
        if (bookingState == BookingState::ACTIVE) {
            activeRooms.insert(roomNumber);
            bookingStateByRoom[roomNumber] = "occupied:" + QString::fromStdString(booking->getBookingId());
            continue;
        }
        if (bookingState != BookingState::UPCOMING || activeRooms.find(roomNumber) != activeRooms.end()) continue;

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

RoomStatusPageWidget::~RoomStatusPageWidget(){
    delete ui;
}
