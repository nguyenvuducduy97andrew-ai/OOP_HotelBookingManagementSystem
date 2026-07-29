#include "ReservationDialog.h"
#include "CountryInputRules.h"
#include "StandardRoom.h"
#include "DeluxeRoom.h"
#include "SuiteRoom.h"
#include "RoomAvailabilityService.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QRegularExpression>

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
        QLineEdit, QComboBox, QDateEdit, QSpinBox {
            background-color: #F4F7FE;
            border: 1px solid #E9EDF7;
            border-radius: 8px;
            padding: 6px 12px;
            font-size: 13px;
            color: #2B3674;
        }
        QComboBox {
            padding-right: 28px;
        }
        QLineEdit:focus, QComboBox:focus, QDateEdit:focus, QSpinBox:focus {
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

    // Modified: Keep reservation ID validation consistent with Customer Management.
    auto* idRow = new QHBoxLayout();
    idRow->setSpacing(8);
    m_customerIdCountry = new QComboBox(this);
    for (const auto& rule : countryInputRules()) {
        m_customerIdCountry->addItem(rule.name, rule.key);
    }
    m_customerIdEdit = new QLineEdit(this);
    idRow->addWidget(m_customerIdCountry, 0);
    idRow->addWidget(m_customerIdEdit, 1);
    formLayout->addRow("Customer ID:", idRow);

    m_customerNameEdit = new QLineEdit(this);
    m_customerNameEdit->setPlaceholderText("Customer name");
    formLayout->addRow("Customer Name:", m_customerNameEdit);

    auto* phoneRow = new QHBoxLayout();
    phoneRow->setSpacing(8);
    m_customerPhoneCode = new QComboBox(this);
    for (const auto& rule : countryInputRules()) {
        m_customerPhoneCode->addItem(QString("%1 (%2)").arg(rule.name, rule.callingCode), rule.key);
    }
    m_customerPhoneLocalEdit = new QLineEdit(this);
    phoneRow->addWidget(m_customerPhoneCode, 0);
    phoneRow->addWidget(m_customerPhoneLocalEdit, 1);
    formLayout->addRow("Phone Number:", phoneRow);
    updateIdPlaceholder();
    updatePhonePlaceholder();

    QDate today = QDate::currentDate();
    m_checkInDateEdit = new QDateEdit(today, this);
    m_checkInDateEdit->setCalendarPopup(true);
    m_checkInDateEdit->setDisplayFormat("yyyy-MM-dd");
    // Modified: Match the service rule by preventing newly created reservations from using historical check-in dates.
    m_checkInDateEdit->setMinimumDate(today);
    formLayout->addRow("Check-in Date:", m_checkInDateEdit);

    m_checkOutDateEdit = new QDateEdit(today.addDays(1), this);
    m_checkOutDateEdit->setCalendarPopup(true);
    m_checkOutDateEdit->setDisplayFormat("yyyy-MM-dd");
    m_checkOutDateEdit->setMinimumDate(today);
    formLayout->addRow("Check-out Date:", m_checkOutDateEdit);

    // Modified: Collect guest occupancy in the reservation so the service can enforce room capacity.
    m_adultCountSpin = new QSpinBox(this);
    m_adultCountSpin->setRange(1, 20);
    m_adultCountSpin->setValue(1);
    formLayout->addRow("Adults:", m_adultCountSpin);

    m_childCountSpin = new QSpinBox(this);
    m_childCountSpin->setRange(0, 20);
    m_childCountSpin->setValue(0);
    formLayout->addRow("Children:", m_childCountSpin);

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
    connect(m_adultCountSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ReservationDialog::updateAvailableRooms);
    connect(m_childCountSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ReservationDialog::updateAvailableRooms);
    connect(m_customerIdCountry, &QComboBox::currentIndexChanged, this, &ReservationDialog::updateIdPlaceholder);
    connect(m_customerPhoneCode, &QComboBox::currentIndexChanged, this, [this]() { updatePhonePlaceholder(); normalizePhoneInput(); });
    connect(m_customerPhoneLocalEdit, &QLineEdit::textChanged, this, &ReservationDialog::normalizePhoneInput);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(saveBtn, &QPushButton::clicked, this, &ReservationDialog::onAccept);
}

void ReservationDialog::updateIdPlaceholder() {
    const auto& rule = countryInputRule(m_customerIdCountry->currentData().toString());
    m_customerIdEdit->setPlaceholderText(rule.idHint);
    m_customerIdEdit->setMaxLength(rule.idMaxLength);
}

void ReservationDialog::updatePhonePlaceholder() {
    const auto& rule = countryInputRule(m_customerPhoneCode->currentData().toString());
    m_customerPhoneLocalEdit->setPlaceholderText(rule.phoneHint);
    m_customerPhoneLocalEdit->setMaxLength(rule.phoneDigits + 1);
}

void ReservationDialog::normalizePhoneInput() {
    // Modified: Store a clean local number before adding the selected dialing code.
    const QString normalized = normalizeLocalPhoneNumber(m_customerPhoneLocalEdit->text());
    if (normalized != m_customerPhoneLocalEdit->text()) {
        m_customerPhoneLocalEdit->setText(normalized);
    }
}

void ReservationDialog::updateAvailableRooms() {
    m_roomCombo->clear();
    if (!m_manager) return;

    QString checkInStr = m_checkInDateEdit->date().toString("yyyy-MM-dd");
    QString checkOutStr = m_checkOutDateEdit->date().toString("yyyy-MM-dd");

    if (checkOutStr <= checkInStr) {
        return;
    }

    std::string availabilityError;
    RoomAvailabilityService availability(*m_manager);
    const auto availableRooms = availability.getAvailableRoomsForDates(
        checkInStr.toStdString(), checkOutStr.toStdString(), availabilityError, m_editingBookingId);
    for (const auto& room : availableRooms) {
        if (m_adultCountSpin->value() + m_childCountSpin->value() > room->getMaximumGuests()) {
            continue;
        }
        // Modified: Render the centrally computed availability list instead of recalculating booking conflicts per room.
        const std::string roomNum = room->getRoomNumber();
        const std::string label = roomNum + " (" + room->getRoomTypeName() + ")";
        m_roomCombo->addItem(QString::fromStdString(label), QString::fromStdString(roomNum));
    }
}

void ReservationDialog::onAccept() {
    const auto& idRule = countryInputRule(m_customerIdCountry->currentData().toString());
    const auto& phoneRule = countryInputRule(m_customerPhoneCode->currentData().toString());
    const QString customerId = m_customerIdEdit->text().trimmed().toUpper();
    const QString customerName = m_customerNameEdit->text().trimmed();
    const QString phoneLocal = normalizeLocalPhoneNumber(m_customerPhoneLocalEdit->text());
    const QString fullPhone = phoneRule.callingCode + phoneLocal;

    m_customerIdEdit->setText(customerId);
    m_customerPhoneLocalEdit->setText(phoneLocal);

    if (!idRule.idPattern.match(customerId).hasMatch()) {
        QMessageBox::warning(this, "Invalid customer ID", QString("Customer ID for %1 must be %2.").arg(idRule.name, idRule.idHint));
        return;
    }

    if (!HotelManager::isValidCustomerNameFormat(customerName.toStdString())) {
        QMessageBox::warning(this, "Invalid customer name", "Customer name must have at least 2 words and include both uppercase and lowercase letters.");
        return;
    }

    static const QRegularExpression digitsPattern(QStringLiteral(R"(^\d+$)"));
    if (!digitsPattern.match(phoneLocal).hasMatch()) {
        QMessageBox::warning(this, "Invalid phone number", "Phone number must contain digits only.");
        return;
    }

    if (phoneLocal.size() != phoneRule.phoneDigits) {
        QMessageBox::warning(this, "Invalid phone number", QString("Phone number for %1 must be %2.").arg(phoneRule.name, phoneRule.phoneHint));
        return;
    }

    if (!HotelManager::isValidPhoneNumberFormat(fullPhone.toStdString())) {
        QMessageBox::warning(this, "Invalid phone number", "Phone number format is invalid.");
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

    if (m_adultCountSpin->value() + m_childCountSpin->value() <= 0) {
        QMessageBox::warning(this, "Invalid guest count", "A reservation must include at least one guest.");
        return;
    }

    accept();
}

QString ReservationDialog::getCustomerId() const {
    return m_customerIdEdit->text().trimmed().toUpper();
}

QString ReservationDialog::getCustomerName() const {
    return m_customerNameEdit->text().trimmed();
}

QString ReservationDialog::getCustomerPhone() const {
    const auto& rule = countryInputRule(m_customerPhoneCode->currentData().toString());
    return rule.callingCode + normalizeLocalPhoneNumber(m_customerPhoneLocalEdit->text());
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

int ReservationDialog::getAdultCount() const {
    return m_adultCountSpin->value();
}

int ReservationDialog::getChildCount() const {
    return m_childCountSpin->value();
}

void ReservationDialog::setEditBooking(const std::string& bookingId) {
    m_editingBookingId = bookingId;
    if (!m_manager) return;

    auto booking = m_manager->findBookingById(bookingId);
    if (!booking) return;

    setWindowTitle("Edit Reservation");
    const QString existingId = QString::fromStdString(booking->getCustomer()->getCustomerId()).trimmed().toUpper();
    for (int i = 0; i < m_customerIdCountry->count(); ++i) {
        const auto& rule = countryInputRule(m_customerIdCountry->itemData(i).toString());
        if (rule.idPattern.match(existingId).hasMatch()) {
            m_customerIdCountry->setCurrentIndex(i);
            break;
        }
    }
    m_customerIdEdit->setText(existingId);
    m_customerIdEdit->setEnabled(false); // Disallow editing Guest ID to protect DB references

    m_customerNameEdit->setText(QString::fromStdString(booking->getCustomer()->getName()));

    const QString existingPhone = QString::fromStdString(booking->getCustomer()->getPhoneNumber()).trimmed();
    bool matchedCode = false;
    for (int i = 0; i < m_customerPhoneCode->count(); ++i) {
        const auto& rule = countryInputRule(m_customerPhoneCode->itemData(i).toString());
        if (existingPhone.startsWith(rule.callingCode)) {
            m_customerPhoneCode->setCurrentIndex(i);
            m_customerPhoneLocalEdit->setText(existingPhone.mid(rule.callingCode.size()));
            matchedCode = true;
            break;
        }
    }
    if (!matchedCode) {
        m_customerPhoneLocalEdit->setText(existingPhone);
    }

    QDate checkIn = QDate::fromString(QString::fromStdString(booking->getCheckInDate()), "yyyy-MM-dd");
    QDate checkOut = QDate::fromString(QString::fromStdString(booking->getCheckOutDate()), "yyyy-MM-dd");

    if (checkIn < QDate::currentDate()) {
        m_checkInDateEdit->setMinimumDate(checkIn);
    }
    m_checkInDateEdit->setDate(checkIn);
    m_checkOutDateEdit->setDate(checkOut);
    m_adultCountSpin->setValue(booking->getAdultCount());
    m_childCountSpin->setValue(booking->getChildCount());
    if (m_manager->getBookingState(*booking) == BookingState::ACTIVE) {
        // Modified: Active stays retain their original arrival date; checkout remains the explicit completion action.
        m_checkInDateEdit->setEnabled(false);
    }

    updateAvailableRooms();

    std::string currentRoom = booking->getRoom()->getRoomNumber();
    int idx = m_roomCombo->findData(QString::fromStdString(currentRoom));
    if (idx >= 0) {
        m_roomCombo->setCurrentIndex(idx);
    }
}

bool ReservationDialog::selectRoom(const std::string& roomNumber) {
    updateAvailableRooms();
    const int roomIndex = m_roomCombo->findData(QString::fromStdString(roomNumber));
    if (roomIndex < 0) {
        return false;
    }

    m_roomCombo->setCurrentIndex(roomIndex);
    return true;
}
