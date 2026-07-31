#include "ReservationDialog.h"
#include "CountryInputRules.h"
#include "CustomerIdentity.h"
#include "Customer.h"
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
#include <QCompleter>

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

    // Modified: Let reservations reuse the stored customer key instead of re-registering a known guest from manually retyped fields.
    m_existingCustomerCombo = new QComboBox(this);
    m_existingCustomerCombo->setEditable(true);
    m_existingCustomerCombo->setInsertPolicy(QComboBox::NoInsert);
    m_existingCustomerCombo->lineEdit()->setPlaceholderText("Search document number, name, or phone...");
    if (auto* completer = m_existingCustomerCombo->completer()) {
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        completer->setFilterMode(Qt::MatchContains);
        completer->setCompletionMode(QCompleter::PopupCompletion);
    }
    populateExistingCustomerPicker();
    formLayout->addRow("Existing customer:", m_existingCustomerCombo);

    // Modified: Keep reservation document validation consistent with Customer Management.
    auto* idRow = new QHBoxLayout();
    idRow->setSpacing(8);
    m_customerDocumentType = new QComboBox(this);
    m_customerDocumentType->addItems({"National ID", "Passport", "Other"});
    m_customerIdCountry = new QComboBox(this);
    for (const auto& rule : countryInputRules()) {
        m_customerIdCountry->addItem(rule.name, rule.key);
    }
    m_customerIdEdit = new QLineEdit(this);
    idRow->addWidget(m_customerDocumentType, 0);
    idRow->addWidget(m_customerIdCountry, 0);
    idRow->addWidget(m_customerIdEdit, 1);
    formLayout->addRow("Identity document:", idRow);

    m_customerNameEdit = new QLineEdit(this);
    m_customerNameEdit->setPlaceholderText("Full legal name as shown on identification");
    formLayout->addRow("Full legal name:", m_customerNameEdit);

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
    connect(m_customerDocumentType, &QComboBox::currentIndexChanged, this, &ReservationDialog::updateIdPlaceholder);
    connect(m_customerIdCountry, &QComboBox::currentIndexChanged, this, &ReservationDialog::updateIdPlaceholder);
    connect(m_customerPhoneCode, &QComboBox::currentIndexChanged, this, [this]() { updatePhonePlaceholder(); normalizePhoneInput(); });
    connect(m_customerPhoneLocalEdit, &QLineEdit::textChanged, this, &ReservationDialog::normalizePhoneInput);
    connect(m_existingCustomerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ReservationDialog::applyExistingCustomerSelection);
    connect(m_existingCustomerCombo->lineEdit(), &QLineEdit::textEdited, this, [this]() {
        if (!m_selectedCustomerId.isEmpty()) {
            // Modified: Return to explicit manual entry if staff changes a selected customer's search text.
            m_selectedCustomerId.clear();
            setCustomerFieldsEnabled(true);
        }
    });
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(saveBtn, &QPushButton::clicked, this, &ReservationDialog::onAccept);
}

void ReservationDialog::updateIdPlaceholder() {
    const auto& rule = countryInputRule(m_customerIdCountry->currentData().toString());
    const QString documentType = m_customerDocumentType->currentText();
    m_customerIdEdit->setPlaceholderText(documentNumberHint(documentType, rule.key));
    m_customerIdEdit->setMaxLength(documentType.compare("Passport", Qt::CaseInsensitive) == 0 ? 20
        : (documentType.compare("Other", Qt::CaseInsensitive) == 0 ? 30 : rule.idMaxLength));
}

void ReservationDialog::populateExistingCustomerPicker()
{
    m_existingCustomerCombo->clear();
    m_existingCustomerCombo->addItem("New customer — enter details manually", QString());
    if (!m_manager) {
        return;
    }

    for (const auto& customer : m_manager->getCustomers()) {
        if (!customer || customer->isArchived()) {
            continue;
        }
        const QString documentNumber = QString::fromStdString(customer->getDocumentNumber());
        const QString name = QString::fromStdString(customer->getName());
        const QString phone = QString::fromStdString(customer->getPhoneNumber());
        const QString label = QString("%1 — %2 — %3").arg(documentNumber, name, phone);
        m_existingCustomerCombo->addItem(label, QString::fromStdString(customer->getCustomerId()));
    }
}

void ReservationDialog::setCustomerFieldsEnabled(bool enabled)
{
    m_customerDocumentType->setEnabled(enabled);
    m_customerIdCountry->setEnabled(enabled);
    m_customerIdEdit->setEnabled(enabled);
    m_customerNameEdit->setEnabled(enabled);
    m_customerPhoneCode->setEnabled(enabled);
    m_customerPhoneLocalEdit->setEnabled(enabled);
}

void ReservationDialog::applyExistingCustomerSelection(int index)
{
    const QString selectedId = m_existingCustomerCombo->itemData(index).toString();
    if (selectedId.isEmpty()) {
        // Modified: Make manual registration an explicit clean branch after staff leaves an existing customer selection.
        m_selectedCustomerId.clear();
        setCustomerFieldsEnabled(true);
        m_customerDocumentType->setCurrentIndex(0);
        m_customerIdCountry->setCurrentIndex(0);
        m_customerIdEdit->clear();
        m_customerNameEdit->clear();
        m_customerPhoneCode->setCurrentIndex(0);
        m_customerPhoneLocalEdit->clear();
        return;
    }
    if (!m_manager) {
        return;
    }

    const auto customer = m_manager->findCustomerById(selectedId.toStdString());
    if (!customer || customer->isArchived()) {
        QMessageBox::warning(this, "Customer unavailable", "The selected customer is no longer available for a reservation.");
        m_existingCustomerCombo->setCurrentIndex(0);
        m_selectedCustomerId.clear();
        setCustomerFieldsEnabled(true);
        return;
    }

    m_selectedCustomerId = selectedId;
    const int documentTypeIndex = m_customerDocumentType->findText(QString::fromStdString(customer->getDocumentType()));
    if (documentTypeIndex >= 0) {
        m_customerDocumentType->setCurrentIndex(documentTypeIndex);
    }
    const int countryIndex = m_customerIdCountry->findData(QString::fromStdString(customer->getIssuingCountry()));
    if (countryIndex >= 0) {
        m_customerIdCountry->setCurrentIndex(countryIndex);
    }
    m_customerIdEdit->setText(QString::fromStdString(customer->getDocumentNumber()));
    m_customerNameEdit->setText(QString::fromStdString(customer->getName()));

    const QString existingPhone = QString::fromStdString(customer->getPhoneNumber());
    int phoneCountryIndex = -1;
    int matchedCodeLength = -1;
    for (int i = 0; i < m_customerPhoneCode->count(); ++i) {
        const auto& rule = countryInputRule(m_customerPhoneCode->itemData(i).toString());
        if (existingPhone.startsWith(rule.callingCode) && rule.callingCode.size() > matchedCodeLength) {
            phoneCountryIndex = i;
            matchedCodeLength = rule.callingCode.size();
        }
    }
    if (phoneCountryIndex >= 0) {
        const auto& rule = countryInputRule(m_customerPhoneCode->itemData(phoneCountryIndex).toString());
        m_customerPhoneCode->setCurrentIndex(phoneCountryIndex);
        m_customerPhoneLocalEdit->setText(existingPhone.mid(rule.callingCode.size()));
    } else {
        m_customerPhoneLocalEdit->setText(existingPhone);
    }
    setCustomerFieldsEnabled(false);
}

void ReservationDialog::updatePhonePlaceholder() {
    const auto& rule = countryInputRule(m_customerPhoneCode->currentData().toString());
    m_customerPhoneLocalEdit->setPlaceholderText(rule.phoneHint);
    m_customerPhoneLocalEdit->setMaxLength(rule.phoneMaxDigits + 1);
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
    const bool usingExistingCustomer = !m_selectedCustomerId.isEmpty();
    const auto& idRule = countryInputRule(m_customerIdCountry->currentData().toString());
    const QString documentType = getDocumentType();
    const auto& phoneRule = countryInputRule(m_customerPhoneCode->currentData().toString());
    const QString customerId = m_customerIdEdit->text().trimmed().toUpper();
    // Modified: Normalize one legal-name field without imposing a Western first/last-name structure.
    const QString customerName = m_customerNameEdit->text().simplified();
    const QString phoneLocal = normalizeLocalPhoneNumber(m_customerPhoneLocalEdit->text());
    const QString fullPhone = phoneRule.callingCode + phoneLocal;

    m_customerIdEdit->setText(customerId);
    m_customerNameEdit->setText(customerName);
    m_customerPhoneLocalEdit->setText(phoneLocal);

    if (!usingExistingCustomer && m_editingBookingId.empty() && !isValidDocumentNumber(documentType, idRule.key, customerId)) {
        QMessageBox::warning(this, "Invalid identity document", QString("%1 for %2 must be %3.").arg(documentType, idRule.name, documentNumberHint(documentType, idRule.key)));
        return;
    }

    if (!usingExistingCustomer && !HotelManager::isValidCustomerNameFormat(customerName.toStdString())) {
        QMessageBox::warning(this, "Invalid customer name", "Enter a valid legal name using letters, spaces, apostrophes, hyphens, or initials.");
        return;
    }

    static const QRegularExpression digitsPattern(QStringLiteral(R"(^\d+$)"));
    if (!usingExistingCustomer && !digitsPattern.match(phoneLocal).hasMatch()) {
        QMessageBox::warning(this, "Invalid phone number", "Phone number must contain digits only.");
        return;
    }

    if (!usingExistingCustomer && (phoneLocal.size() < phoneRule.phoneMinDigits || phoneLocal.size() > phoneRule.phoneMaxDigits)) {
        QMessageBox::warning(this, "Invalid phone number", QString("Phone number for %1 must be %2.").arg(phoneRule.name, phoneRule.phoneHint));
        return;
    }

    if (!usingExistingCustomer && !HotelManager::isValidPhoneNumberFormat(fullPhone.toStdString())) {
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
    if (!m_editingBookingId.empty() && m_manager) {
        const auto booking = m_manager->findBookingById(m_editingBookingId);
        if (booking && booking->getCustomer()) {
            return QString::fromStdString(booking->getCustomer()->getCustomerId());
        }
    }
    if (!m_selectedCustomerId.isEmpty()) {
        return m_selectedCustomerId;
    }
    return customerIdentityKey(getDocumentType(), getIssuingCountry(), getDocumentNumber());
}

QString ReservationDialog::getDocumentType() const { return m_customerDocumentType->currentText(); }
QString ReservationDialog::getIssuingCountry() const { return m_customerIdCountry->currentData().toString(); }
QString ReservationDialog::getDocumentNumber() const { return m_customerIdEdit->text().trimmed().toUpper(); }

QString ReservationDialog::getCustomerName() const {
    if (!m_selectedCustomerId.isEmpty() && m_manager) {
        if (const auto customer = m_manager->findCustomerById(m_selectedCustomerId.toStdString())) {
            return QString::fromStdString(customer->getName());
        }
    }
    return m_customerNameEdit->text().simplified();
}

QString ReservationDialog::getCustomerPhone() const {
    if (!m_selectedCustomerId.isEmpty() && m_manager) {
        if (const auto customer = m_manager->findCustomerById(m_selectedCustomerId.toStdString())) {
            return QString::fromStdString(customer->getPhoneNumber());
        }
    }
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
    const QString existingId = QString::fromStdString(booking->getCustomer()->getDocumentNumber()).trimmed().toUpper();
    const QString existingType = QString::fromStdString(booking->getCustomer()->getDocumentType());
    const QString existingCountry = QString::fromStdString(booking->getCustomer()->getIssuingCountry());
    const int typeIndex = m_customerDocumentType->findText(existingType);
    if (typeIndex >= 0) {
        m_customerDocumentType->setCurrentIndex(typeIndex);
    }
    for (int i = 0; i < m_customerIdCountry->count(); ++i) {
        if (m_customerIdCountry->itemData(i).toString() == existingCountry) {
            m_customerIdCountry->setCurrentIndex(i);
            break;
        }
    }
    m_customerIdEdit->setText(existingId);
    m_customerIdEdit->setEnabled(false); // Disallow editing Guest ID to protect DB references
    m_customerDocumentType->setEnabled(false);
    m_customerIdCountry->setEnabled(false);

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
