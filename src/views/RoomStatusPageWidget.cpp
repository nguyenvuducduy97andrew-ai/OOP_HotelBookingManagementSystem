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
    ui->dateEditCheckIn->setDate(m_selectedCheckIn.date());
    ui->dateEditCheckOut->setDate(m_selectedCheckOut.date());

    auto* filterLayout = qobject_cast<QGridLayout*>(ui->frameFilterPanel->layout());
    if (filterLayout) {
        ui->lblCheckInTitle->setText("Booking schedule");
        ui->lblCheckOutTitle->hide();
        ui->dateEditCheckIn->hide();
        ui->dateEditCheckOut->hide();
        m_scheduleButton = new QPushButton("Choose dates & times", ui->frameFilterPanel);
        m_scheduleButton->setObjectName("btnChooseSchedule");
        // Modified: Give the shared schedule entry a dedicated visible treatment instead of inheriting an empty legacy date-field cell.
        m_scheduleButton->setMinimumHeight(42);
        m_scheduleButton->setMinimumWidth(150);
        m_scheduleButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_scheduleButton->setStyleSheet(
            "QPushButton { background:#EFF6FF; color:#1D4ED8; border:1px solid #BFDBFE; "
            "border-radius:10px; padding:9px 14px; font-weight:700; text-align:left; }"
            "QPushButton:hover { background:#DBEAFE; border-color:#60A5FA; }");
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

        connect(m_scheduleButton, &QPushButton::clicked, this, [this]() { openSchedulePicker(false); });
        connect(m_checkInScheduleField, &QPushButton::clicked, this, [this]() { openSchedulePicker(false); });
        connect(m_checkOutScheduleField, &QPushButton::clicked, this, [this]() { openSchedulePicker(true); });
        connect(m_roomTypeCombo, &QComboBox::currentIndexChanged, this, [this](int) {
            setFilterType(m_roomTypeCombo->currentData().toString());
        });
    }

    connect(ui->btnAddAdult, &QPushButton::clicked, this, [this]() {
        int val = ui->lblAdultCount->text().toInt();
        ui->lblAdultCount->setText(QString::number(val + 1));
        applyFilters();
    });
    connect(ui->btnMinusAdult, &QPushButton::clicked, this, [this]() {
        int val = ui->lblAdultCount->text().toInt();
        if (val > 1) ui->lblAdultCount->setText(QString::number(val - 1));
        applyFilters();
    });

    connect(ui->btnAddChild, &QPushButton::clicked, this, [this]() {
        int val = ui->lblChildrenCount->text().toInt();
        ui->lblChildrenCount->setText(QString::number(val + 1));
        applyFilters();
    });
    connect(ui->btnMinusChild, &QPushButton::clicked, this, [this]() {
        int val = ui->lblChildrenCount->text().toInt();
        if (val > 0) ui->lblChildrenCount->setText(QString::number(val - 1));
        applyFilters();
    });

    // Room type filtering now lives in the schedule row instead of a separate button row.
    ui->btnFilterAll->hide();
    ui->btnFilterStandard->hide();
    ui->btnFilterDeluxe->hide();
    ui->btnFilterSuite->hide();

    // Connect the search field.
    connect(ui->txtSearchRoom, &QLineEdit::textChanged, this, &RoomStatusPageWidget::applyFilters);
    addSearchIcon(ui->txtSearchRoom);
    // Keep the original horizontal span while reducing the search bar's vertical footprint.
    ui->txtSearchRoom->setFixedHeight(32);

    // Keep button sizes fixed so the layout does not shift with shorter labels.
    ui->btnCheckAvailability->setMinimumWidth(190);
    ui->btnCheckAvailability->setMaximumWidth(220);
    ui->btnCheckAvailability->setMinimumHeight(42);

    // Connect the Check Availability button.
    connect(ui->btnCheckAvailability, &QPushButton::clicked, this, [this]() {
        setAvailabilityMode(!m_isCheckAvailMode);
    });

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
            if (!m_isCheckAvailMode || !m_selectedCheckIn.isValid() || !m_selectedCheckOut.isValid()) {
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
        // Modified: Keep the availability action concise so it remains fully readable at narrow window widths.
        ui->btnCheckAvailability->setText("Clear filter");
        ui->btnCheckAvailability->setStyleSheet(
            "QPushButton { background-color: #E53935; color: white; border-radius: 10px; padding: 10px; font-weight: bold; }"
            "QPushButton:hover { background-color: #C62828; }"
        );
    } else {
        ui->btnCheckAvailability->setText("Check availability");
        ui->btnCheckAvailability->setStyleSheet("");
    }
    applyFilters();
}

void RoomStatusPageWidget::applyFilters() {
    const QString type = m_roomTypeCombo ? m_roomTypeCombo->currentData().toString() : QString("All");

    QString searchText = ui->txtSearchRoom->text().trimmed().toLower();
    QGridLayout* gridLayout = qobject_cast<QGridLayout*>(ui->scrollAreaWidgetContents->layout());
    if (!gridLayout) return;

    // Modified: Batch card reparenting and visibility changes so changing a filter or returning from the booking dialog does not visibly stutter.
    ui->scrollAreaWidgetContents->setUpdatesEnabled(false);

    clearFloorSections();

    const QDateTime reqIn = m_selectedCheckIn;
    const QDateTime reqOut = m_selectedCheckOut;
    int reqAdults = ui->lblAdultCount->text().toInt();
    int reqChildren = ui->lblChildrenCount->text().toInt();

    std::unordered_set<std::string> availableRoomNumbers;
    if (m_isCheckAvailMode) {
        std::string availabilityError;
        const auto availableRooms = m_manager->getAvailableRoomsForPeriod(
            reqIn.toString(Qt::ISODate).toStdString(),
            reqOut.toString(Qt::ISODate).toStdString(), availabilityError);
        for (const auto& room : availableRooms) {
            if (room) {
                availableRoomNumbers.insert(room->getRoomNumber());
            }
        }
    }

    QMap<int, QList<RoomCard*>> visibleCardsByFloor;

    for (RoomCard* card : m_roomCards) {
        bool matchType = (type == "All" || card->getRoomType() == type);
        bool matchSearch = true;
        bool matchAvail = true;

        if (!searchText.isEmpty()) {
            matchSearch = card->getGuestName().toLower().contains(searchText) ||
                          card->getRoomNumber().toLower().contains(searchText) ||
                          card->getRoomType().toLower().contains(searchText) ||
                          card->getDateIn().toLower().contains(searchText);
        }

        if (m_isCheckAvailMode) {
            // Modified: Use the shared booking-and-maintenance availability result so Room Status cannot disagree with Reservation.
            matchAvail = availableRoomNumbers.find(card->getRoomNumber().toStdString()) != availableRoomNumbers.end();

            if (matchAvail) {
                const auto room = m_manager->findRoomByNumber(card->getRoomNumber().toStdString());
                // Modified: Read the configured capacity from the canonical room instead of assuming a capacity from its room type.
                if (!room || reqAdults <= 0 || reqChildren < 0
                    || reqAdults + reqChildren > room->getMaximumGuests()) {
                    matchAvail = false;
                }
            }
            
            if (matchAvail) {
                card->setTempAvailMode(true);
            } else {
                card->setTempAvailMode(false);
            }
        } else {
            card->setTempAvailMode(false);
        }

        if (matchType && matchSearch && matchAvail) {
            visibleCardsByFloor[floorForRoomNumber(card->getRoomNumber())].append(card);
        } else {
            card->hide(); 
        }
    }

    const int columns = floorColumnCount();
    m_lastFloorColumnCount = columns;
    int sectionRow = 0;
    for (auto floorIt = visibleCardsByFloor.begin(); floorIt != visibleCardsByFloor.end(); ++floorIt) {
        auto cards = floorIt.value();
        std::sort(cards.begin(), cards.end(), [](RoomCard* left, RoomCard* right) {
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
            "QFrame#floorSection { background:#FFFFFF; border:1px solid #E2E8F0; border-radius:14px; }"
            "QLabel#floorLabel { color:#1D4ED8; background:#EFF6FF; border:1px solid #BFDBFE; "
            "border-radius:10px; font-size:16px; font-weight:800; padding:12px 8px; }");
        auto* sectionLayout = new QHBoxLayout(section);
        sectionLayout->setContentsMargins(12, 12, 12, 12);
        sectionLayout->setSpacing(14);

        auto* floorLabel = new QLabel(
            floorIt.key() > 0 ? QString("Floor %1").arg(floorIt.key()) : QString("Other"), section);
        floorLabel->setObjectName("floorLabel");
        floorLabel->setAlignment(Qt::AlignCenter);
        floorLabel->setFixedWidth(86);
        floorLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        sectionLayout->addWidget(floorLabel);

        auto* roomsWidget = new QWidget(section);
        auto* roomsLayout = new QGridLayout(roomsWidget);
        roomsLayout->setContentsMargins(0, 0, 0, 0);
        roomsLayout->setHorizontalSpacing(10);
        roomsLayout->setVerticalSpacing(10);
        roomsLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        for (int index = 0; index < cards.size(); ++index) {
            RoomCard* card = cards.at(index);
            card->show();
            roomsLayout->addWidget(card, index / columns, index % columns,
                                   Qt::AlignTop | Qt::AlignLeft);
        }
        sectionLayout->addWidget(roomsWidget, 1);
        gridLayout->addWidget(section, sectionRow++, 0);
        m_floorSections.append(section);
    }
    gridLayout->setColumnStretch(0, 1);
    ui->scrollAreaWidgetContents->setUpdatesEnabled(true);
    ui->scrollAreaWidgetContents->update();
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

    if (auto* gridLayout = qobject_cast<QGridLayout*>(ui->scrollAreaWidgetContents->layout())) {
        QLayoutItem* item = nullptr;
        while ((item = gridLayout->takeAt(0)) != nullptr) delete item;
    }
}

void RoomStatusPageWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    const int columns = floorColumnCount();
    if (!m_roomCards.isEmpty() && columns != m_lastFloorColumnCount) {
        QTimer::singleShot(0, this, &RoomStatusPageWidget::applyFilters);
    }
}

QString RoomStatusPageWidget::roomStateSignature() const
{
    if (!m_manager) return {};

    const QDateTime now = QDateTime::currentDateTime();
    QStringList stateParts;
    stateParts.reserve(static_cast<int>(m_manager->getRooms().size()));

    for (const auto& room : m_manager->getRooms()) {
        if (!room || room->isArchived()) continue;

        QString state = "available";
        if (!room->getIsAvailable()
            || m_manager->isRoomBlockedAt(room->getRoomNumber(), now.toString(Qt::ISODateWithMs).toStdString())) {
            state = "maintenance";
        } else {
            for (const auto& booking : m_manager->getBookings()) {
                if (!booking || booking->isCancelled() || booking->isDeleted()
                    || !booking->getRoom()
                    || booking->getRoom()->getRoomNumber() != room->getRoomNumber()) {
                    continue;
                }

                const BookingState bookingState = m_manager->getBookingState(*booking);
                if (bookingState == BookingState::ACTIVE) {
                    state = "occupied:" + QString::fromStdString(booking->getBookingId());
                    break;
                }
                if (bookingState == BookingState::UPCOMING) {
                    QDateTime plannedStart = QDateTime::fromString(
                        QString::fromStdString(booking->getPlannedCheckInAt()), Qt::ISODateWithMs);
                    QDateTime plannedEnd = QDateTime::fromString(
                        QString::fromStdString(booking->getPlannedCheckOutAt()), Qt::ISODateWithMs);
                    if (!plannedStart.isValid()) {
                        plannedStart = QDateTime(
                            QDate::fromString(QString::fromStdString(booking->getCheckInDate()), Qt::ISODate),
                            QTime(0, 0));
                    }
                    if (!plannedEnd.isValid()) {
                        plannedEnd = QDateTime(
                            QDate::fromString(QString::fromStdString(booking->getCheckOutDate()), Qt::ISODate),
                            QTime(0, 0));
                    }
                    if (plannedStart.isValid() && plannedEnd.isValid()
                        && plannedStart <= now && now < plannedEnd) {
                        state = "awaiting:" + QString::fromStdString(booking->getBookingId());
                        break;
                    }
                }
            }
        }
        stateParts.append(QString::fromStdString(room->getRoomNumber()) + '=' + state);
    }
    return stateParts.join('|');
}

RoomStatusPageWidget::~RoomStatusPageWidget(){
    delete ui;
}
