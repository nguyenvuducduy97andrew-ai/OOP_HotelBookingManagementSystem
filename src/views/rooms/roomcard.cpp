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
    if (event->button() == Qt::LeftButton) {
        event->accept();
    } else {
        QWidget::mousePressEvent(event);
    }
}

void RoomCard::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (rect().contains(event->position().toPoint())) {
            emit cardClicked();
        }
        event->accept();
    } else {
        QWidget::mouseReleaseEvent(event);
    }
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
    m_isTempAvail = false;
    
    // Clear data
    m_guestName = ""; m_idNumber = ""; m_phoneNumber = ""; m_dateIn = ""; m_dateOut = "";

    ui->frameStatusBadge->setStyleSheet("QFrame#frameStatusBadge { background-color: #05CD99; border-radius: 10px; }");
    ui->frameCardContainer->setStyleSheet("QFrame#frameCardContainer { background-color: #eafff1; border: 1px solid #E9EDF7; border-radius: 16px; }");

    ui->lblGuestName->setStyleSheet("color: #05CD99; background-color: transparent; font-size: 18px; font-weight: bold; border: none;");
    ui->lblCheckOutDate->hide();
    ui->lblCheckInDate->hide();
    ui->lineDivider->hide();
    ui->lblGuestName->hide();

    ui->lblStatusIcon->setPixmap(QPixmap(":/avl_icon.png"));
    ui->lblStatusIcon->setScaledContents(true);
}

void RoomCard::setAwaiting(QString guestName, QString dateIn, QString dateOut) {
    // Modified: Render reserved arrivals distinctly so a room is not mistaken for sellable inventory before check-in.
    m_status = "AWT";
    m_isTempAvail = false;
    m_guestName = guestName;
    m_idNumber = "";
    m_phoneNumber = "";
    m_dateIn = dateIn;
    m_dateOut = dateOut;

    ui->lblCheckOutDate->show();
    ui->lblCheckInDate->show();
    ui->lineDivider->show();
    ui->lblGuestName->show();
    ui->lblGuestName->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    ui->frameStatusBadge->setStyleSheet("QFrame#frameStatusBadge { background-color: #3B58FF; border-radius: 10px; }");
    ui->frameCardContainer->setStyleSheet("QFrame#frameCardContainer { background-color: #e7e7fb; border: 1px solid #E9EDF7; border-radius: 16px; }");
    ui->lblGuestName->setStyleSheet("color: #1B2559; background: transparent; font-size: 14px; font-weight: bold; border: none;");
    ui->lblCheckInDate->setStyleSheet("color: #A3AED0; font-size: 14px; background: transparent;");
    ui->lblCheckOutDate->setStyleSheet("color: #A3AED0; font-size: 14px; background: transparent;");

    ui->lblGuestName->setText(guestName);
    ui->lblCheckInDate->setText("IN: " + dateIn);
    ui->lblCheckOutDate->setText("OUT: " + dateOut);
    ui->lblStatusIcon->setText("");
    ui->lblStatusIcon->setStyleSheet("background: transparent;");
    ui->lblStatusIcon->setPixmap(QPixmap(":/awt_icon.png"));
    ui->lblStatusIcon->setScaledContents(true);
}

void RoomCard::setOccupied(QString guestName, QString idNumber, QString phoneNumber, QString dateIn, QString dateOut) { // Render the occupied state.
    m_status = "OCC";
    m_isTempAvail = false;
    
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

    ui->frameStatusBadge->setStyleSheet("QFrame#frameStatusBadge { background-color: #FFB547; border-radius: 10px; }");
    ui->frameCardContainer->setStyleSheet("QFrame#frameCardContainer { background-color: #fffbee; border: 1px solid #E9EDF7; border-radius: 16px; }");

    ui->lblGuestName->setStyleSheet("color: #1B2559; background: transparent; font-size: 14px; font-weight: bold; border: none;");
    ui->lblCheckInDate->setStyleSheet("color: #A3AED0; font-size: 14px; background: transparent;");
    ui->lblCheckOutDate->setStyleSheet("color: #A3AED0; font-size: 14px; background: transparent;");

    ui->lblGuestName->setText(guestName);
    ui->lblCheckInDate->setText ("IN: " + dateIn);
    ui->lblCheckOutDate->setText("OUT: " + dateOut);

    ui->lblStatusIcon->setPixmap(QPixmap(":/occ_icon.png"));
    ui->lblStatusIcon->setScaledContents(true);
}


void RoomCard::setMaintenance() {
    m_status = "MTN";
    m_isTempAvail = false;

    ui->lblCheckOutDate->hide();
    ui->lblCheckInDate->hide();
    ui->lineDivider->hide();

    ui->frameStatusBadge->setStyleSheet("QFrame#frameStatusBadge { background-color: #A3AED0; border-radius: 10px; }");
    ui->frameCardContainer->setStyleSheet("QFrame#frameCardContainer { background-color: #f0f5ff; border: 1px solid #E9EDF7; border-radius: 16px; }");

    ui->lblGuestName->setStyleSheet("color: #1B2559; background: transparent; font-size: 16px; font-weight: bold; border: none;");
    ui->lblGuestName->setText("");

    ui->lblStatusIcon->setPixmap(QPixmap(":/mtn_icon.png"));
    ui->lblStatusIcon->setScaledContents(true);
}

bool RoomCard::isTempAvailMode() const {
    return m_isTempAvail;
}

void RoomCard::setTempAvailMode(bool enabled) {
    if (!enabled && !m_isTempAvail) {
        return;
    }

    if (!enabled) {
        // Modified: Restore the physical state after a future-period availability preview, including Awaiting and Maintenance cards.
        m_isTempAvail = false;
        if (m_status == "OCC") {
            setOccupied(m_guestName, m_idNumber, m_phoneNumber, m_dateIn, m_dateOut);
        } else if (m_status == "AWT") {
            setAwaiting(m_guestName, m_dateIn, m_dateOut);
        } else if (m_status == "MTN") {
            setMaintenance();
        }
        return;
    }

    if (m_status == "AVL") {
        m_isTempAvail = false;
        return;
    }

    // Modified: A room can be sellable in a future schedule even when its current physical card is Occupied, Awaiting, or under Maintenance.
    m_isTempAvail = true;
    if (enabled) {
        // Temporarily show it as available (green) while keeping the existing layout.
        ui->lblCheckOutDate->hide();
        ui->lblCheckInDate->hide();
        ui->lineDivider->hide();
        ui->lblGuestName->setText("");
        
        ui->frameStatusBadge->setStyleSheet("QFrame#frameStatusBadge { background-color: #05CD99; border-radius: 10px; }");
        ui->frameCardContainer->setStyleSheet("QFrame#frameCardContainer { background-color: #ecffee; border: 1px solid #E9EDF7; border-radius: 16px; }");

        ui->lblGuestName->setStyleSheet("color: #1B2559; background: transparent; font-size: 18px; font-weight: bold; border: none;");
        
        ui->lblStatusIcon->setPixmap(QPixmap(":/avl_icon.png"));
        ui->lblStatusIcon->setScaledContents(true);
    }
}
