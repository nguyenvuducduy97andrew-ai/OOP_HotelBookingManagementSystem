#include "ReservationDialog.h"
#include "StandardRoom.h"
#include "DeluxeRoom.h"
#include "SuiteRoom.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QMessageBox>

ReservationDialog::ReservationDialog(HotelManager* manager, QWidget *parent)
    : QDialog(parent), m_manager(manager), m_editingBookingId("") {
    setupUI();
    setWindowTitle("New Reservation");
    updateAvailableRooms();
}

void setupReservationDialogStyle(QDialog* dialog) {
    dialog->setStyleSheet(R"(
        QDialog {
            background-color: #FFFFFF;
        }
        QLabel {
            font-size: 13px;
            color: #2B3674;
            font-weight: 600;
        }
        QLineEdit, QComboBox, QDateEdit {
            background-color: #F4F7FE;
            border: 1px solid #E9EDF7;
            border-radius: 8px;
            padding: 6px 12px;
            font-size: 13px;
            color: #2B3674;
        }
        QLineEdit:focus, QComboBox:focus, QDateEdit:focus {
            border: 1px solid #005BFE;
        }
        QComboBox QAbstractItemView {
            background-color: #FFFFFF;
            color: #2B3674;
            selection-background-color: #005BFE;
            selection-color: #FFFFFF;
            border: 1px solid #E9EDF7;
        }
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
        QPushButton#btnSave {
            background-color: #005BFE;
            color: #FFFFFF;
            font-weight: 600;
            border-radius: 8px;
            padding: 8px 16px;
            font-size: 13px;
        }
        QPushButton#btnSave:hover {
            background-color: #2B7BFF;
        }
        QPushButton#btnCancel {
            background-color: #E9EDF7;
            color: #2B3674;
            font-weight: 600;
            border-radius: 8px;
            padding: 8px 16px;
            font-size: 13px;
            border: none;
        }
        QPushButton#btnCancel:hover {
            background-color: #D3DDF4;
        }
    )");
}

void ReservationDialog::setupUI() {
    setupReservationDialogStyle(this);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    auto* formLayout = new QFormLayout();
    formLayout->setSpacing(12);

    m_customerIdEdit = new QLineEdit(this);
    m_customerIdEdit->setPlaceholderText("ID card number or Guest ID");
    formLayout->addRow("Customer ID (CCCD):", m_customerIdEdit);

    m_customerNameEdit = new QLineEdit(this);
    m_customerNameEdit->setPlaceholderText("Customer name");
    formLayout->addRow("Customer Name:", m_customerNameEdit);

    m_customerPhoneEdit = new QLineEdit(this);
    m_customerPhoneEdit->setPlaceholderText("Phone number");
    formLayout->addRow("Phone Number:", m_customerPhoneEdit);

    QDate today = QDate::currentDate();
    m_checkInDateEdit = new QDateEdit(today, this);
    m_checkInDateEdit->setCalendarPopup(true);
    m_checkInDateEdit->setDisplayFormat("yyyy-MM-dd");
    m_checkInDateEdit->setMinimumDate(today.addDays(-30)); // Allow past checking for demo but recommend modern
    formLayout->addRow("Check-in Date:", m_checkInDateEdit);

    m_checkOutDateEdit = new QDateEdit(today.addDays(1), this);
    m_checkOutDateEdit->setCalendarPopup(true);
    m_checkOutDateEdit->setDisplayFormat("yyyy-MM-dd");
    m_checkOutDateEdit->setMinimumDate(today);
    formLayout->addRow("Check-out Date:", m_checkOutDateEdit);

    m_roomCombo = new QComboBox(this);
    formLayout->addRow("Available Rooms:", m_roomCombo);

    mainLayout->addLayout(formLayout);

    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    auto* cancelBtn = new QPushButton("Cancel", this);
    cancelBtn->setObjectName("btnCancel");
    auto* saveBtn = new QPushButton("Book Room", this);
    saveBtn->setObjectName("btnSave");

    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(saveBtn);
    mainLayout->addLayout(btnLayout);

    connect(m_checkInDateEdit, &QDateEdit::dateChanged, this, &ReservationDialog::updateAvailableRooms);
    connect(m_checkOutDateEdit, &QDateEdit::dateChanged, this, &ReservationDialog::updateAvailableRooms);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(saveBtn, &QPushButton::clicked, this, &ReservationDialog::onAccept);
}

void ReservationDialog::updateAvailableRooms() {
    m_roomCombo->clear();
    if (!m_manager) return;

    QString checkInStr = m_checkInDateEdit->date().toString("yyyy-MM-dd");
    QString checkOutStr = m_checkOutDateEdit->date().toString("yyyy-MM-dd");

    if (checkOutStr <= checkInStr) {
        return;
    }

    for (const auto& room : m_manager->getRooms()) {
        if (!room || !room->getIsAvailable()) continue; // Skip rooms under maintenance

        std::string roomNum = room->getRoomNumber();

        // Check if overlaps with any active bookings
        bool isFree = true;
        for (const auto& booking : m_manager->getBookings()) {
            if (!booking || booking->isCancelled() || booking->isDeleted()) continue;
            if (booking->getBookingId() == m_editingBookingId) continue;
            auto roomPtr = booking->getRoom();
            if (!roomPtr || roomPtr->getRoomNumber() != roomNum) continue;

            // Overlap check
            if (checkInStr.toStdString() < booking->getCheckOutDate() &&
                booking->getCheckInDate() < checkOutStr.toStdString()) {
                isFree = false;
                break;
            }
        }

        if (isFree) {
            // Fixed-modified: Format room labels from the virtual type name.
            std::string label = roomNum;
            // Add subclass type suffix
            // e.g. "101 (Standard)", "301 (Suite)"
            label += " (" + room->getRoomTypeName() + ")";

            m_roomCombo->addItem(QString::fromStdString(label), QString::fromStdString(roomNum));
        }
    }
}

void ReservationDialog::onAccept() {
    if (m_customerIdEdit->text().trimmed().isEmpty() ||
        m_customerNameEdit->text().trimmed().isEmpty() ||
        m_customerPhoneEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Missing information", "Please fill out the guest information completely.");
        return;
    }

    if (m_roomCombo->currentIndex() < 0) {
        QMessageBox::warning(this, "No available room", "No available rooms are available for the selected date range.");
        return;
    }

    if (m_checkOutDateEdit->date() <= m_checkInDateEdit->date()) {
        QMessageBox::warning(this, "Invalid date", "Check-out date must be after check-in date.");
        return;
    }

    accept();
}

QString ReservationDialog::getCustomerId() const {
    return m_customerIdEdit->text().trimmed();
}

QString ReservationDialog::getCustomerName() const {
    return m_customerNameEdit->text().trimmed();
}

QString ReservationDialog::getCustomerPhone() const {
    return m_customerPhoneEdit->text().trimmed();
}

QString ReservationDialog::getRoomNumber() const {
    return m_roomCombo->currentData().toString();
}

QString ReservationDialog::getCheckInDate() const {
    return m_checkInDateEdit->date().toString("yyyy-MM-dd");
}

QString ReservationDialog::getCheckOutDate() const {
    return m_checkOutDateEdit->date().toString("yyyy-MM-dd");
}

void ReservationDialog::setEditBooking(const std::string& bookingId) {
    m_editingBookingId = bookingId;
    if (!m_manager) return;

    auto booking = m_manager->findBookingById(bookingId);
    if (!booking) return;

    setWindowTitle("Edit Reservation");
    m_customerIdEdit->setText(QString::fromStdString(booking->getCustomer()->getCustomerId()));
    m_customerIdEdit->setEnabled(false); // Disallow editing Guest ID to protect DB references

    m_customerNameEdit->setText(QString::fromStdString(booking->getCustomer()->getName()));
    m_customerPhoneEdit->setText(QString::fromStdString(booking->getCustomer()->getPhoneNumber()));

    QDate checkIn = QDate::fromString(QString::fromStdString(booking->getCheckInDate()), "yyyy-MM-dd");
    QDate checkOut = QDate::fromString(QString::fromStdString(booking->getCheckOutDate()), "yyyy-MM-dd");

    m_checkInDateEdit->setDate(checkIn);
    m_checkOutDateEdit->setDate(checkOut);

    updateAvailableRooms();

    std::string currentRoom = booking->getRoom()->getRoomNumber();
    int idx = m_roomCombo->findData(QString::fromStdString(currentRoom));
    if (idx >= 0) {
        m_roomCombo->setCurrentIndex(idx);
    }
}
