#include "RoomStatusPageWidget.h"
#include "ui_RoomStatusPageWidget.h"
#include "StandardRoom.h"
#include "DeluxeRoom.h"
#include "SuiteRoom.h"
#include "Customer.h"
#include "bookingdialog.h"
#include "CustomConfirmDialog.h"
#include <QGridLayout>
#include <QDate>
#include <QDate>
#include <QMessageBox>
#include "DataManager.h"

RoomStatusPageWidget::RoomStatusPageWidget(HotelManager* manager, QWidget *parent)
    : QWidget(parent), ui(new Ui::RoomStatusPageWidget), m_manager(manager) {
    ui->setupUi(this);
    
    // Ngăn chặn SearchBar tự động Focus
    this->setFocusPolicy(Qt::StrongFocus);
    this->setFocus();

    ui->lblAdultCount->setText("0");
    ui->lblChildrenCount->setText("0");

    // Đặt ngày mặc định là hôm nay
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

    // Kết nối các nút Filter
    connect(ui->btnFilterAll, &QPushButton::clicked, this, [=]() { setFilterType("All"); });
    connect(ui->btnFilterStandard, &QPushButton::clicked, this, [=]() { setFilterType("Standard"); });
    connect(ui->btnFilterDeluxe, &QPushButton::clicked, this, [=]() { setFilterType("Deluxe"); });
    connect(ui->btnFilterSuite, &QPushButton::clicked, this, [=]() { setFilterType("Suite"); });

    // Kết nối thanh tìm kiếm
    connect(ui->txtSearchRoom, &QLineEdit::textChanged, this, &RoomStatusPageWidget::applyFilters);

    // Kết nối nút Check Availability
    connect(ui->btnCheckAvailability, &QPushButton::clicked, this, [=]() {
        m_isCheckAvailMode = !m_isCheckAvailMode;
        if (m_isCheckAvailMode) {
            ui->btnCheckAvailability->setText("Clear Filter");
            ui->btnCheckAvailability->setStyleSheet("background-color: #E53935; color: white; border-radius: 10px; padding: 10px; font-weight: bold;");
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

    for (const auto& room : m_manager->getRooms()) {
        if (!room) continue;

        RoomCard* card = new RoomCard(ui->scrollAreaWidgetContents);
        card->setRoomNumber(QString::fromStdString(room->getRoomNumber()));

        QString roomType = "Standard";
        if (dynamic_cast<DeluxeRoom*>(room.get())) roomType = "Deluxe";
        else if (dynamic_cast<SuiteRoom*>(room.get())) roomType = "Suite";
        card->setRoomType(roomType);

        bool isOccupied = false;
        std::shared_ptr<Booking> activeBooking = nullptr;
        for (const auto& booking : m_manager->getBookings()) {
            if (booking && !booking->isCancelled() && !booking->isDeleted() && booking->getRoom() &&
                booking->getRoom()->getRoomNumber() == room->getRoomNumber()) {
                if (m_manager->getBookingState(*booking) == BookingState::ACTIVE) {
                    isOccupied = true;
                    activeBooking = booking;
                    break;
                }
            }
        }

        if (!room->getIsAvailable()) {
            card->setMaintenance();
        } else if (isOccupied && activeBooking) {
            QString guestName = activeBooking->getCustomer() ? QString::fromStdString(activeBooking->getCustomer()->getName()) : "Unknown";
            QString phone = activeBooking->getCustomer() ? QString::fromStdString(activeBooking->getCustomer()->getPhoneNumber()) : "";
            QString idNumber = activeBooking->getCustomer() ? QString::fromStdString(activeBooking->getCustomer()->getCustomerId()) : "";
            
            QDate dIn = QDate::fromString(QString::fromStdString(activeBooking->getCheckInDate()), "yyyy-MM-dd");
            QString dateIn = dIn.isValid() ? dIn.toString("dd/MM") : QString::fromStdString(activeBooking->getCheckInDate());
            
            QDate dOut = QDate::fromString(QString::fromStdString(activeBooking->getCheckOutDate()), "yyyy-MM-dd");
            QString dateOut = dOut.isValid() ? dOut.toString("dd/MM") : QString::fromStdString(activeBooking->getCheckOutDate());
            
            card->setOccupied(guestName, idNumber, phone, dateIn, dateOut);
        } else {
            card->setAvailable();
        }

        m_roomCards.append(card);

        // Click event to interact
        connect(card, &RoomCard::cardClicked, this, [=]() {
            if (card->getStatus() == "MTN") {
                // If it is in maintenance, allow to restore it
                CustomConfirmDialog dialog("Confirm return to service", QString("Complete maintenance for room %1 and return it to service?").arg(QString::fromStdString(room->getRoomNumber())), false, this);
                if (dialog.exec() == QDialog::Accepted && dialog.isConfirmed()) {
                    room->setIsAvailable(true);
                    refreshData();
                }
                return;
            }

            BookingDialog form(this);

            // Nếu phòng đã có khách và không phải đang lọc chỗ trống -> Mở chế độ Edit
            if (card->getStatus() == "OCC" && !card->isTempAvailMode()) {
                form.setEditMode(true);
                form.setGuestData(card->getGuestName(), card->getIdNumber(), card->getPhoneNumber(), card->getDateIn(), card->getDateOut());
            } 
            // Nếu phòng trống (hoặc đang mượn UI trống) -> Mở chế độ Check-in
            else {
                form.setEditMode(false);
            }

            // Nếu user điền hợp lệ và bấm Confirm/Edit
            if (form.exec() == QDialog::Accepted) {
                QString guestName = form.getGuestName();
                QString idNumber = form.getIdNumber();
                QString phone = form.getPhoneNumber();
                
                // form.getDateIn() trả về "dd/MM", ta cần "yyyy-MM-dd" cho HotelManager
                QString currentYear = QString::number(QDate::currentDate().year());
                QDate dIn = QDate::fromString(form.getDateIn() + "/" + currentYear, "dd/MM/yyyy");
                QDate dOut = QDate::fromString(form.getDateOut() + "/" + currentYear, "dd/MM/yyyy");
                
                std::string err;
                
                if (card->getStatus() == "OCC" && !card->isTempAvailMode()) {
                    // Update existing booking
                    if (activeBooking) {
                        m_manager->registerCustomer(idNumber.toStdString(), guestName.toStdString(), phone.toStdString(), err);
                        m_manager->updateBooking(activeBooking->getBookingId(), idNumber.toStdString(), room->getRoomNumber(), dIn.toString("yyyy-MM-dd").toStdString(), dOut.toString("yyyy-MM-dd").toStdString(), err);
                    }
                } else {
                    // Create new booking
                    m_manager->registerCustomer(idNumber.toStdString(), guestName.toStdString(), phone.toStdString(), err);
                    m_manager->createBooking(idNumber.toStdString(), room->getRoomNumber(), dIn.toString("yyyy-MM-dd").toStdString(), dOut.toString("yyyy-MM-dd").toStdString(), err);
                }
                
                DataManager::getInstance().saveAll(*m_manager);
                
                card->setTempAvailMode(false); // Thoát khỏi trạng thái ảo
                refreshData(); // Cập nhật lại UI từ DB
            }
        });

        int row = index / columns;
        int col = index % columns;
        gridLayout->addWidget(card, row, col, Qt::AlignTop | Qt::AlignLeft);
        index++;
    }

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
            if (card->getStatus() == "MTN") {
                matchAvail = false;
            } else if (card->getStatus() == "OCC") {
                // Approximate logic for checking availability (dates in MM/yyyy for current year)
                int currentYear = QDate::currentDate().year();
                QDate cardIn = QDate::fromString(card->getDateIn() + QString("/%1").arg(currentYear), "dd/MM/yyyy");
                QDate cardOut = QDate::fromString(card->getDateOut() + QString("/%1").arg(currentYear), "dd/MM/yyyy");
                
                if (cardIn.isValid() && cardOut.isValid()) {
                    if (!(reqOut <= cardIn || reqIn >= cardOut)) {
                        matchAvail = false;
                    }
                } else {
                    matchAvail = false;
                }
            }

            if (matchAvail) {
                int maxAdults = 2, maxChildren = 1;
                if (card->getRoomType() == "Deluxe") { maxAdults = 2; maxChildren = 2; }
                else if (card->getRoomType() == "Suite") { maxAdults = 4; maxChildren = 2; }

                if (reqAdults > maxAdults || reqChildren > maxChildren) {
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
