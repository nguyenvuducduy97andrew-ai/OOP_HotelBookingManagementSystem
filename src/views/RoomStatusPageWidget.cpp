#include "RoomStatusPageWidget.h"
#include "ui_RoomStatusPageWidget.h"
#include "Customer.h"
#include "RoomManager.h"
#include "RoomInfoDialog.h"
#include <QGridLayout>
#include <QDate>
#include <unordered_map>
#include <unordered_set>

RoomStatusPageWidget::RoomStatusPageWidget(HotelManager* manager, QWidget *parent)
    : QWidget(parent), ui(new Ui::RoomStatusPageWidget), m_manager(manager) {
    ui->setupUi(this);
    
    // Prevent the search bar from receiving focus automatically.
    this->setFocusPolicy(Qt::StrongFocus);
    this->setFocus();

    ui->lblAdultCount->setText("0");
    ui->lblChildrenCount->setText("0");

    // Set today's date as the default.
    ui->dateEditCheckIn->setDate(QDate::currentDate());
    ui->dateEditCheckOut->setDate(QDate::currentDate().addDays(1));

    connect(ui->btnAddAdult, &QPushButton::clicked, this, [=]() {
        int val = ui->lblAdultCount->text().toInt();
        ui->lblAdultCount->setText(QString::number(val + 1));
    });
    connect(ui->btnMinusAdult, &QPushButton::clicked, this, [=]() {
        int val = ui->lblAdultCount->text().toInt();
        if (val > 0) ui->lblAdultCount->setText(QString::number(val - 1));
    });

    connect(ui->btnAddChild, &QPushButton::clicked, this, [=]() {
        int val = ui->lblChildrenCount->text().toInt();
        ui->lblChildrenCount->setText(QString::number(val + 1));
    });
    connect(ui->btnMinusChild, &QPushButton::clicked, this, [=]() {
        int val = ui->lblChildrenCount->text().toInt();
        if (val > 0) ui->lblChildrenCount->setText(QString::number(val - 1));
    });

    // Connect the filter buttons.
    connect(ui->btnFilterAll, &QPushButton::clicked, this, [=]() { setFilterType("All"); });
    connect(ui->btnFilterStandard, &QPushButton::clicked, this, [=]() { setFilterType("Standard"); });
    connect(ui->btnFilterDeluxe, &QPushButton::clicked, this, [=]() { setFilterType("Deluxe"); });
    connect(ui->btnFilterSuite, &QPushButton::clicked, this, [=]() { setFilterType("Suite"); });

    // Connect the search field.
    connect(ui->txtSearchRoom, &QLineEdit::textChanged, this, &RoomStatusPageWidget::applyFilters);

    // Keep button sizes fixed so the layout does not shift with shorter labels.
    ui->btnCheckAvailability->setFixedWidth(180);

    // Connect the Check Availability button.
    connect(ui->btnCheckAvailability, &QPushButton::clicked, this, [=]() {
        m_isCheckAvailMode = !m_isCheckAvailMode;
        if (m_isCheckAvailMode) {
            ui->btnCheckAvailability->setText("Clear Filter");
            ui->btnCheckAvailability->setStyleSheet(
                "QPushButton { background-color: #E53935; color: white; border-radius: 10px; padding: 10px; font-weight: bold; }"
                "QPushButton:hover { background-color: #C62828; }"
            );
        } else {
            ui->btnCheckAvailability->setText("Check Availability");
            ui->btnCheckAvailability->setStyleSheet(""); 
        }
        applyFilters();
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
}

void RoomStatusPageWidget::setupUI() {
    // Currently merged into constructor
}

void RoomStatusPageWidget::refreshData() {
    if (!m_manager) return;

    QGridLayout* gridLayout = qobject_cast<QGridLayout*>(ui->scrollAreaWidgetContents->layout());
    if (!gridLayout) return;

    // Clear old cards
    for (RoomCard* card : m_roomCards) {
        gridLayout->removeWidget(card);
        delete card;
    }
    m_roomCards.clear();

    const int columns = 4;
    int index = 0;

    std::unordered_map<std::string, std::shared_ptr<Booking>> activeBookingsByRoom;
    std::unordered_map<std::string, std::shared_ptr<Booking>> awaitingBookingsByRoom;
    const std::string today = QDate::currentDate().toString(Qt::ISODate).toStdString();
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
        } else if (state == BookingState::UPCOMING
                   && booking->getCheckInDate() <= today && today < booking->getCheckOutDate()) {
            // Modified: A reservation due today (or overdue) holds inventory but is not physically occupied until check-in.
            awaitingBookingsByRoom.emplace(bookedRoom->getRoomNumber(), booking);
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
            || m_manager->isRoomUnderMaintenance(room->getRoomNumber(), today);
        if (isUnderMaintenance) {
            card->setMaintenance();
        } else if (isOccupied && activeBooking) {
            QString guestName = activeBooking->getCustomer() ? QString::fromStdString(activeBooking->getCustomer()->getName()) : "Unknown";
            QString phone = activeBooking->getCustomer() ? QString::fromStdString(activeBooking->getCustomer()->getPhoneNumber()) : "";
            QString idNumber = activeBooking->getCustomer() ? QString::fromStdString(activeBooking->getCustomer()->getDocumentNumber()) : "";
            
            QDate dIn = QDate::fromString(QString::fromStdString(activeBooking->getCheckInDate()), "yyyy-MM-dd");
            QString dateIn = dIn.isValid() ? dIn.toString("dd/MM") : QString::fromStdString(activeBooking->getCheckInDate());
            
            QDate dOut = QDate::fromString(QString::fromStdString(activeBooking->getCheckOutDate()), "yyyy-MM-dd");
            QString dateOut = dOut.isValid() ? dOut.toString("dd/MM") : QString::fromStdString(activeBooking->getCheckOutDate());
            
            card->setOccupied(guestName, idNumber, phone, dateIn, dateOut);
        } else if (awaitingBooking) {
            const auto customer = awaitingBooking->getCustomer();
            const QString guestName = customer ? QString::fromStdString(customer->getName()) : "Awaiting check-in";
            const QDate plannedCheckIn = QDate::fromString(QString::fromStdString(awaitingBooking->getCheckInDate()), Qt::ISODate);
            const QDate plannedCheckOut = QDate::fromString(QString::fromStdString(awaitingBooking->getCheckOutDate()), Qt::ISODate);
            const QString dateIn = plannedCheckIn.isValid() ? plannedCheckIn.toString("dd/MM")
                : QString::fromStdString(awaitingBooking->getCheckInDate());
            const QString dateOut = plannedCheckOut.isValid() ? plannedCheckOut.toString("dd/MM")
                : QString::fromStdString(awaitingBooking->getCheckOutDate());
            card->setAwaiting(guestName, dateIn, dateOut);
        } else {
            // Modified: Available now means no active stay or arrival-due reservation holds the room today.
            card->setAvailable();
        }

        m_roomCards.append(card);

        connect(card, &RoomCard::cardClicked, this, [this, room, card]() {
            // Modified: Let Room Status select the room while Reservation owns booking validation and persistence.
            if (card->getStatus() == "AVL") {
                RoomInfoDialog infoDialog(room, this);
                if (infoDialog.exec() == QDialog::Accepted) {
                    QDate reqIn = ui->dateEditCheckIn->date();
                    QDate reqOut = ui->dateEditCheckOut->date();
                    int reqAdults = ui->lblAdultCount->text().toInt();
                    int reqChildren = ui->lblChildrenCount->text().toInt();
                    emit bookingRequested(QString::fromStdString(room->getRoomNumber()), reqIn, reqOut, reqAdults, reqChildren);
                }
            }
        });

        int row = index / columns;
        int col = index % columns;
        gridLayout->addWidget(card, row, col, Qt::AlignTop | Qt::AlignLeft);
        index++;
    }

    // Modified: Index active bookings once so rendering room cards no longer scans every booking for every room.
    applyFilters();
}

void RoomStatusPageWidget::setFilterType(QString type) {
    ui->btnFilterAll->setChecked(type == "All");
    ui->btnFilterStandard->setChecked(type == "Standard");
    ui->btnFilterDeluxe->setChecked(type == "Deluxe");
    ui->btnFilterSuite->setChecked(type == "Suite");
    applyFilters();
}

void RoomStatusPageWidget::applyFilters() {
    QString type = "All";
    if (ui->btnFilterStandard->isChecked()) type = "Standard";
    else if (ui->btnFilterDeluxe->isChecked()) type = "Deluxe";
    else if (ui->btnFilterSuite->isChecked()) type = "Suite";

    QString searchText = ui->txtSearchRoom->text().trimmed().toLower();
    QGridLayout* gridLayout = qobject_cast<QGridLayout*>(ui->scrollAreaWidgetContents->layout());
    if (!gridLayout) return;

    QLayoutItem* item;
    while ((item = gridLayout->takeAt(0)) != nullptr) {
        if (item->spacerItem()) delete item;
    }

    QDate reqIn = ui->dateEditCheckIn->date();
    QDate reqOut = ui->dateEditCheckOut->date();
    int reqAdults = ui->lblAdultCount->text().toInt();
    int reqChildren = ui->lblChildrenCount->text().toInt();

    std::unordered_set<std::string> availableRoomNumbers;
    if (m_isCheckAvailMode) {
        std::string availabilityError;
        const auto availableRooms = m_manager->getAvailableRoomsForDates(
            reqIn.toString(Qt::ISODate).toStdString(),
            reqOut.toString(Qt::ISODate).toStdString(), availabilityError);
        for (const auto& room : availableRooms) {
            if (room) {
                availableRoomNumbers.insert(room->getRoomNumber());
            }
        }
    }

    int visibleIndex = 0;
    int columns = 4;

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
                int maximumGuests = 2;
                if (card->getRoomType() == "Deluxe") { maximumGuests = 3; }
                else if (card->getRoomType() == "Suite") { maximumGuests = 4; }

                // Modified: Keep Room Status capacity filtering consistent with the booking service's room limits.
                if (reqAdults <= 0 || reqChildren < 0 || reqAdults + reqChildren > maximumGuests) {
                    matchAvail = false;
                }
            }
            
            if (matchAvail && card->getStatus() == "OCC") {
                card->setTempAvailMode(true);
            } else {
                card->setTempAvailMode(false);
            }
        } else {
            card->setTempAvailMode(false);
        }

        if (matchType && matchSearch && matchAvail) {
            card->show(); 
            int row = visibleIndex / columns;
            int col = visibleIndex % columns;
            gridLayout->addWidget(card, row, col, Qt::AlignTop | Qt::AlignLeft);
            visibleIndex++;
        } else {
            card->hide(); 
        }
    }

    // Add horizontal spacer to push all cards to the left
    QSpacerItem* horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    gridLayout->addItem(horizontalSpacer, 0, columns, 1, 1);
}

RoomStatusPageWidget::~RoomStatusPageWidget(){
    delete ui;
}
