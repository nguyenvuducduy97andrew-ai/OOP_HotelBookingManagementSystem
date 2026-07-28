#include "roomcard.h"
#include "ui_roomcard.h"

RoomCard::RoomCard(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::RoomCard)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_StyledBackground, true);
    this->setCursor(Qt::PointingHandCursor);
}

RoomCard::~RoomCard()
{
    delete ui;
}

std::string RoomCard::getStatus() const {
    return m_status;
}

void RoomCard::mousePressEvent(QMouseEvent* event) {
    this->move(this->x() + 2, this->y() + 2);
}

void RoomCard::mouseReleaseEvent(QMouseEvent* event) {
    this->move(this->x() - 2, this->y() - 2);
    emit cardClicked();
}


void RoomCard::setRoomNumber(QString roomNumber) { // Store the room number for card rendering.
    m_roomNumber = roomNumber;
    ui->lblRoomNumber->setText(roomNumber);
    ui->lblRoomNumber->setStyleSheet("color: white;");
    ui->lblRoomType->setStyleSheet("color: white;");
}

void RoomCard::setRoomType(QString roomType) { // Store the room type for card rendering.
    m_roomType = roomType;
    ui->lblRoomType->setText(roomType);
}


QString RoomCard::getRoomType() const { return m_roomType; }
QString RoomCard::getRoomNumber() const { return m_roomNumber; }
QString RoomCard::getGuestName() const { return m_guestName; }
QString RoomCard::getIdNumber() const { return m_idNumber; }
QString RoomCard::getPhoneNumber() const { return m_phoneNumber; }
QString RoomCard::getDateIn() const { return m_dateIn; }
QString RoomCard::getDateOut() const { return m_dateOut; }

void RoomCard::setAvailable() { // Render the available state.
    m_status = "AVL";
    
    // Clear data
    m_guestName = ""; m_idNumber = ""; m_phoneNumber = ""; m_dateIn = ""; m_dateOut = "";

    ui->frameStatusBadge->setStyleSheet("background-color: #05A660; border-radius: 10px;");
    ui->frameCardContainer->setStyleSheet("background-color: #E2FBE8; border-radius: 10px;");

    ui->lblGuestName->setStyleSheet("color: #05A660; background-color: transparent; font-size: 18px; font-weight: bold; border: none;");
    ui->lblCheckOutDate->hide();
    ui->lblCheckInDate->hide();
    ui->lineDivider->hide();
    ui->lblGuestName->hide();

    ui->lblStatusIcon->setPixmap(QPixmap(":/avl_icon.png"));
    ui->lblStatusIcon->setScaledContents(true);
}

void RoomCard::setOccupied(QString guestName, QString idNumber, QString phoneNumber, QString dateIn, QString dateOut) { // Render the occupied state.
    m_status = "OCC";
    
    // Store data.
    m_guestName = guestName;
    m_idNumber = idNumber;
    m_phoneNumber = phoneNumber;
    m_dateIn = dateIn;
    m_dateOut = dateOut;

    ui->lblCheckOutDate->show();
    ui->lblCheckInDate->show();
    ui->lineDivider->show();
    ui->lblGuestName->show();
    ui->lblGuestName->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    ui->frameStatusBadge->setStyleSheet("background-color: #E53935; border-radius: 10px;");
    ui->frameCardContainer->setStyleSheet("background-color: #FFE1E1; border-radius: 10px;");

    ui->lblGuestName->setStyleSheet("color: black; background: transparent; font-size: 14px; font-weight: bold; border: none;");
    ui->lblCheckInDate->setStyleSheet("color: black; font-size: 14px; background: transparent;");
    ui->lblCheckOutDate->setStyleSheet("color: black; font-size: 14px; background: transparent;");

    ui->lblGuestName->setText(guestName);
    ui->lblCheckInDate->setText ("IN: " + dateIn);
    ui->lblCheckOutDate->setText("OUT: " + dateOut);

    ui->lblStatusIcon->setPixmap(QPixmap(":/occ_icon.png"));
    ui->lblStatusIcon->setScaledContents(true);
}


void RoomCard::setMaintenance() {
    m_status = "MTN";

    ui->lblCheckOutDate->hide();
    ui->lblCheckInDate->hide();
    ui->lineDivider->hide();

    ui->frameStatusBadge->setStyleSheet("background-color: #757575; border-radius: 10px;");
    ui->frameCardContainer->setStyleSheet("background-color: #E0E0E0; border-radius: 10px;");

    ui->lblGuestName->setStyleSheet("color: #0f172a; background: transparent; font-size: 16px; font-weight: bold; border: none;");
    ui->lblGuestName->setText("");

    ui->lblStatusIcon->setPixmap(QPixmap(":/mtn_icon.png"));
    ui->lblStatusIcon->setScaledContents(true);
}

bool RoomCard::isTempAvailMode() const {
    return m_isTempAvail;
}

void RoomCard::setTempAvailMode(bool enabled) {
    if (m_status != "OCC") return; // Applies only to occupied (red) rooms.

    m_isTempAvail = enabled;
    
    if (enabled) {
        // Temporarily show it as available (green) while keeping the existing layout.
        ui->lblCheckOutDate->hide();
        ui->lblCheckInDate->hide();
        ui->lineDivider->hide();
        ui->lblGuestName->setText("");
        
        ui->frameStatusBadge->setStyleSheet("background-color: #05A660; border-radius: 10px;");
        ui->frameCardContainer->setStyleSheet("background-color: #E2FBE8; border-radius: 10px;");
        
        ui->lblGuestName->setStyleSheet("color: #0f172a; background: transparent; font-size: 18px; font-weight: bold; border: none;");
        
        ui->lblStatusIcon->setPixmap(QPixmap(":/avl_icon.png"));
        ui->lblStatusIcon->setScaledContents(true);
    } else {
        // Restore the occupied (red) state.
        ui->lblCheckOutDate->show();
        ui->lblCheckInDate->show();
        ui->lineDivider->show();
        ui->lblGuestName->show();
        ui->lblGuestName->setText(m_guestName);
        
        ui->frameStatusBadge->setStyleSheet("background-color: #E53935; border-radius: 10px;");
        ui->frameCardContainer->setStyleSheet("background-color: #FFE1E1; border-radius: 10px;");
        
        ui->lblGuestName->setStyleSheet("color: black; background: transparent; font-size: 16px; font-weight: bold; border: none;");
        
        ui->lblStatusIcon->setPixmap(QPixmap(":/occ_icon.png"));
        ui->lblStatusIcon->setScaledContents(true);
    }
}
