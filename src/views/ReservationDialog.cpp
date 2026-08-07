#include "ReservationDialog.h"
#include "CountryInputRules.h"
#include "CustomerIdentity.h"
#include "Customer.h"
#include "StandardRoom.h"
#include "DeluxeRoom.h"
#include "SuiteRoom.h"
#include "SchedulePickerDialog.h"
#include "CustomConfirmDialog.h"
#include "DialogWindowBehavior.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QRegularExpression>
#include <QCompleter>
#include <QButtonGroup>
#include <QLocale>
#include <QSignalBlocker>
#include <QScrollArea>
#include <QStackedLayout>
#include <QTimeEdit>
#include <algorithm>

ReservationDialog::ReservationDialog(HotelManager* manager, QWidget *parent)
    : QDialog(parent), m_manager(manager), m_editingBookingId("") {
    setupUI();
    setWindowTitle("New Reservation");
    updateAvailableRooms();
}

void setupReservationDialogStyle(QDialog* dialog) {
    // Modified: Keep Reservation opaque while replacing the system title bar with a soft in-surface header.
    dialog->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    dialog->setStyleSheet(R"(
        QDialog {
            background-color: #FFFFFF;
            border: 2px solid #93C5FD;
            border-radius: 18px;
        }
        QLabel {
            font-size: 13px;
            color: #2B3674;
            font-weight: 600;
            background: transparent;
            border: none;
        }
        QLineEdit, QComboBox, QDateEdit, QTimeEdit, QSpinBox {
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
        QLineEdit:focus, QComboBox:focus, QDateEdit:focus, QTimeEdit:focus, QSpinBox:focus {
            border: 1px solid #3B58FF;
        }
        QComboBox QAbstractItemView {
            background-color: #FFFFFF;
            color: #2B3674;
            selection-background-color: #3B58FF;
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
            selection-background-color: #3B58FF;
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
            background-color: #3B58FF;
            color: #FFFFFF;
            font-weight: 600;
            border-radius: 8px;
            padding: 8px 16px;
            font-size: 13px;
        }
        QPushButton#btnSave:hover {
            background-color: #4F6BFF;
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
        QPushButton#btnSchedule {
            color: #3B58FF;
            background-color: #EFF6FF;
            border: 1px solid #BFDBFE;
            border-radius: 8px;
            font-weight: 700;
            padding: 8px 12px;
        }
        QPushButton#btnSchedule:hover {
            background-color: #DBEAFE;
        }
        QLabel#validationFeedback {
            color: #92400E;
            background-color: #FFFBEB;
            border: 1px solid #FDE68A;
            border-radius: 9px;
            padding: 9px 12px;
            font-weight: 600;
        }
        QLabel#validationFeedback[active="false"] {
            background: transparent;
            border: none;
            padding: 0;
        }
        QLabel#roomReview {
            color: #2B3674;
            background-color: #F4F7FE;
            border: 1px solid #D9E2F2;
            border-radius: 9px;
            padding: 10px 12px;
            font-weight: 500;
        }
        QFrame#customerDetailsContainer {
            background: #F8FAFC;
            border: 1px solid #E2EAF6;
            border-radius: 10px;
            padding: 8px;
        }
        QLabel#reservationDialogTitle {
            font-size: 20px;
            font-weight: 800;
            color: #1B3F83;
        }
        QPushButton#reservationDialogClose {
            background: transparent;
            color: #64748B;
            border: none;
            border-radius: 8px;
            font-size: 20px;
        }
        QPushButton#reservationDialogClose:hover { background: #F1F5F9; color: #1B3F83; }
        QLabel#reservationDialogSubtitle {
            font-size: 12px;
            color: #6B7FA8;
            font-weight: 500;
        }
    )");
}

void ReservationDialog::setupUI() {
    setupReservationDialogStyle(this);
    // Modified: Keep Reservation at a stable usable size; long sub-sections scroll inside the dialog rather than resizing the outer window.
    lockDialogToWorkingArea(this, QSize(780, 700));

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    // Modified: Frameless reservation dialogs retain an in-surface title and explanation instead of relying on a harsh native title bar.
    auto* titleRow = new QHBoxLayout();
    auto* dialogIcon = new QLabel("▣", this);
    dialogIcon->setStyleSheet("color:#3B58FF; font-size:17px; background:transparent; border:none;");
    auto* dialogTitle = new QLabel("Create reservation", this);
    dialogTitle->setObjectName("reservationDialogTitle");
    auto* dialogClose = new QPushButton("×", this);
    dialogClose->setObjectName("reservationDialogClose");
    dialogClose->setFixedSize(30, 30);
    titleRow->addWidget(dialogIcon);
    titleRow->addSpacing(8);
    titleRow->addWidget(dialogTitle);
    titleRow->addStretch();
    titleRow->addWidget(dialogClose);
    enableDialogHeaderDrag(this, dialogTitle);
    auto* dialogSubtitle = new QLabel("Choose the guest, stay schedule, occupancy, and room before saving.", this);
    dialogSubtitle->setObjectName("reservationDialogSubtitle");
    dialogSubtitle->setWordWrap(true);
    mainLayout->addLayout(titleRow);
    mainLayout->addWidget(dialogSubtitle);

    auto* formScroll = new QScrollArea(this);
    formScroll->setWidgetResizable(true);
    formScroll->setFrameShape(QFrame::NoFrame);
    formScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto* formContent = new QWidget(formScroll);
    auto* formLayout = new QFormLayout(formContent);
    formLayout->setContentsMargins(2, 2, 8, 2);
    formLayout->setSpacing(12);

    auto* customerTypeRow = new QHBoxLayout();
    customerTypeRow->setSpacing(8);
    auto* existingCustomerMode = new QPushButton("Existing customer", formContent);
    auto* newCustomerMode = new QPushButton("New customer", formContent);
    existingCustomerMode->setObjectName("btnSchedule");
    newCustomerMode->setObjectName("btnSchedule");
    existingCustomerMode->setCheckable(true);
    newCustomerMode->setCheckable(true);
    auto* customerModeGroup = new QButtonGroup(this);
    customerModeGroup->setExclusive(true);
    customerModeGroup->addButton(existingCustomerMode, 0);
    customerModeGroup->addButton(newCustomerMode, 1);
    newCustomerMode->setChecked(true);
    customerTypeRow->addWidget(existingCustomerMode);
    customerTypeRow->addWidget(newCustomerMode);
    customerTypeRow->addStretch();
    formLayout->addRow("Customer type:", customerTypeRow);

    // Modified: Keep Existing and New customer content in equally sized pages so changing the choice never changes the outer dialog geometry.
    auto* customerDetails = new QFrame(formContent);
    customerDetails->setObjectName("customerDetailsContainer");
    customerDetails->setFixedHeight(188);
    m_customerDetailsStack = new QStackedLayout(customerDetails);
    m_customerDetailsStack->setContentsMargins(0, 0, 0, 0);

    auto* existingCustomerPage = new QWidget(customerDetails);
    auto* existingCustomerLayout = new QVBoxLayout(existingCustomerPage);
    existingCustomerLayout->setContentsMargins(0, 0, 0, 0);
    existingCustomerLayout->setSpacing(8);
    auto* existingHint = new QLabel("Search the saved customer list. Selected profile details remain read-only.", existingCustomerPage);
    existingHint->setObjectName("reservationDialogSubtitle");
    existingHint->setWordWrap(true);
    existingCustomerLayout->addWidget(existingHint);

    // Modified: Let reservations reuse the stored customer key instead of re-registering a known guest from manually retyped fields.
    m_existingCustomerCombo = new QComboBox(existingCustomerPage);
    m_existingCustomerCombo->setEditable(true);
    m_existingCustomerCombo->setInsertPolicy(QComboBox::NoInsert);
    m_existingCustomerCombo->lineEdit()->setPlaceholderText("Search document number, name, or phone...");
    if (auto* completer = m_existingCustomerCombo->completer()) {
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        completer->setFilterMode(Qt::MatchContains);
        completer->setCompletionMode(QCompleter::PopupCompletion);
    }
    populateExistingCustomerPicker();
    existingCustomerLayout->addWidget(m_existingCustomerCombo);
    m_selectedCustomerProfileLabel = new QLabel(existingCustomerPage);
    m_selectedCustomerProfileLabel->setObjectName("roomReview");
    m_selectedCustomerProfileLabel->setWordWrap(true);
    m_selectedCustomerProfileLabel->setFixedHeight(80);
    m_selectedCustomerProfileLabel->setText("Select a customer to show the saved identity and phone details here.");
    existingCustomerLayout->addWidget(m_selectedCustomerProfileLabel);
    existingCustomerLayout->addStretch();

    auto* newCustomerPage = new QWidget(customerDetails);
    auto* newCustomerLayout = new QFormLayout(newCustomerPage);
    newCustomerLayout->setContentsMargins(0, 0, 0, 0);
    newCustomerLayout->setHorizontalSpacing(12);
    newCustomerLayout->setVerticalSpacing(10);

    // Modified: Keep reservation document validation consistent with Customer Management.
    auto* idRowWidget = new QWidget(newCustomerPage);
    auto* idRow = new QHBoxLayout(idRowWidget);
    idRow->setContentsMargins(0, 0, 0, 0);
    idRow->setSpacing(8);
    m_customerDocumentType = new QComboBox(newCustomerPage);
    m_customerDocumentType->addItems({"National ID", "Passport", "Other"});
    m_customerIdCountry = new QComboBox(newCustomerPage);
    for (const auto& rule : countryInputRules()) {
        m_customerIdCountry->addItem(rule.name, rule.key);
    }
    m_customerIdEdit = new QLineEdit(newCustomerPage);
    idRow->addWidget(m_customerDocumentType, 0);
    idRow->addWidget(m_customerIdCountry, 0);
    idRow->addWidget(m_customerIdEdit, 1);
    auto* identityCaption = new QLabel("Identity document:", newCustomerPage);
    newCustomerLayout->addRow(identityCaption, idRowWidget);

    m_customerNameEdit = new QLineEdit(newCustomerPage);
    m_customerNameEdit->setPlaceholderText("Full legal name as shown on identification");
    auto* nameCaption = new QLabel("Full legal name:", newCustomerPage);
    newCustomerLayout->addRow(nameCaption, m_customerNameEdit);

    auto* phoneRowWidget = new QWidget(newCustomerPage);
    auto* phoneRow = new QHBoxLayout(phoneRowWidget);
    phoneRow->setContentsMargins(0, 0, 0, 0);
    phoneRow->setSpacing(8);
    m_customerPhoneCode = new QComboBox(newCustomerPage);
    for (const auto& rule : countryInputRules()) {
        m_customerPhoneCode->addItem(QString("%1 (%2)").arg(rule.name, rule.callingCode), rule.key);
    }
    m_customerPhoneLocalEdit = new QLineEdit(newCustomerPage);
    phoneRow->addWidget(m_customerPhoneCode, 0);
    phoneRow->addWidget(m_customerPhoneLocalEdit, 1);
    auto* phoneCaption = new QLabel("Phone Number:", newCustomerPage);
    newCustomerLayout->addRow(phoneCaption, phoneRowWidget);
    updateIdPlaceholder();
    updatePhonePlaceholder();
    m_customerDetailsStack->addWidget(existingCustomerPage);
    m_customerDetailsStack->addWidget(newCustomerPage);
    m_customerDetailsStack->setCurrentIndex(1);
    formLayout->addRow("Customer details:", customerDetails);

    QDateTime defaultCheckIn = QDateTime::currentDateTime().addSecs(60 * 60);
    defaultCheckIn.setTime(QTime(defaultCheckIn.time().hour(), 0));
    if (defaultCheckIn <= QDateTime::currentDateTime()) {
        defaultCheckIn = defaultCheckIn.addSecs(60 * 60);
    }
    const QDate today = QDate::currentDate();
    m_checkInDateEdit = new QDateEdit(defaultCheckIn.date(), this);
    m_checkInDateEdit->setCalendarPopup(true);
    m_checkInDateEdit->setDisplayFormat("yyyy-MM-dd");
    // Modified: Match the service rule by preventing newly created reservations from using historical check-in dates.
    m_checkInDateEdit->setMinimumDate(today);
    // Modified: The shared schedule picker is the only visible schedule editor; legacy field widgets remain data holders and must not leak into the dialog at (0, 0).
    m_checkInDateEdit->hide();

    m_checkInTimeEdit = new QTimeEdit(defaultCheckIn.time(), this);
    m_checkInTimeEdit->setDisplayFormat("HH:mm");
    m_checkInTimeEdit->setTimeRange(QTime(0, 0), QTime(23, 0));
    m_checkInTimeEdit->setCurrentSection(QDateTimeEdit::HourSection);
    m_checkInTimeEdit->hide();

    const QDateTime defaultCheckOut = defaultCheckIn.addSecs(60 * 60);
    m_checkOutDateEdit = new QDateEdit(defaultCheckOut.date(), this);
    m_checkOutDateEdit->setCalendarPopup(true);
    m_checkOutDateEdit->setDisplayFormat("yyyy-MM-dd");
    m_checkOutDateEdit->setMinimumDate(today);
    m_checkOutDateEdit->hide();

    m_checkOutTimeEdit = new QTimeEdit(defaultCheckOut.time(), this);
    m_checkOutTimeEdit->setDisplayFormat("HH:mm");
    m_checkOutTimeEdit->setTimeRange(QTime(0, 0), QTime(23, 0));
    m_checkOutTimeEdit->setCurrentSection(QDateTimeEdit::HourSection);
    m_checkOutTimeEdit->hide();
    auto* scheduleRow = new QHBoxLayout();
    scheduleRow->setSpacing(8);
    m_scheduleButton = new QPushButton("Choose dates & times", this);
    m_scheduleButton->setObjectName("btnSchedule");
    m_scheduleSummary = new QLabel(this);
    m_scheduleSummary->setWordWrap(true);
    scheduleRow->addWidget(m_scheduleButton, 0);
    scheduleRow->addWidget(m_scheduleSummary, 1);
    formLayout->addRow("Booking schedule:", scheduleRow);
    updateScheduleSummary();

    // Modified: Collect guest occupancy in the reservation so the service can enforce room capacity.
    m_adultCountSpin = new QSpinBox(this);
    m_adultCountSpin->setRange(1, 20);
    m_adultCountSpin->setValue(1);
    formLayout->addRow("Adults:", m_adultCountSpin);

    m_childCountSpin = new QSpinBox(this);
    m_childCountSpin->setRange(0, 20);
    m_childCountSpin->setValue(0);
    formLayout->addRow("Children:", m_childCountSpin);

    auto* roomRow = new QHBoxLayout();
    roomRow->setSpacing(8);
    m_roomCombo = new QComboBox(this);
    roomRow->addWidget(m_roomCombo, 1);
    // Modified: Selecting a room in the list is the single confirmation gesture; a second Select button caused redundant, error-prone state.
    m_confirmRoomButton = nullptr;
    formLayout->addRow("Available Rooms:", roomRow);
    m_roomReviewLabel = new QLabel(this);
    m_roomReviewLabel->setObjectName("roomReview");
    m_roomReviewLabel->setWordWrap(true);
    formLayout->addRow("Room review:", m_roomReviewLabel);

    formScroll->setWidget(formContent);
    mainLayout->addWidget(formScroll, 1);

    m_validationLabel = new QLabel(this);
    m_validationLabel->setObjectName("validationFeedback");
    m_validationLabel->setWordWrap(true);
    // Modified: Reserve one feedback line so guidance can appear without changing Reservation Dialog height.
    m_validationLabel->setFixedHeight(44);
    m_validationLabel->setProperty("active", false);
    m_validationLabel->setText(QString());
    mainLayout->addWidget(m_validationLabel);

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
    connect(m_checkInTimeEdit, &QTimeEdit::timeChanged, this, &ReservationDialog::updateAvailableRooms);
    connect(m_checkOutTimeEdit, &QTimeEdit::timeChanged, this, &ReservationDialog::updateAvailableRooms);
    connect(m_scheduleButton, &QPushButton::clicked, this, &ReservationDialog::openSchedulePicker);
    // Modified: Planned reservations use the agreed 24 whole-hour slots; actual arrival and departure still preserve seconds.
    const auto keepWholeHour = [](QTimeEdit* editor, const QTime& time) {
        if (time.minute() != 0 || time.second() != 0) {
            editor->setTime(QTime(time.hour(), 0));
        }
    };
    connect(m_checkInTimeEdit, &QTimeEdit::timeChanged, this, [this, keepWholeHour](const QTime& time) {
        keepWholeHour(m_checkInTimeEdit, time);
    });
    connect(m_checkOutTimeEdit, &QTimeEdit::timeChanged, this, [this, keepWholeHour](const QTime& time) {
        keepWholeHour(m_checkOutTimeEdit, time);
    });
    connect(m_adultCountSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ReservationDialog::updateAvailableRooms);
    connect(m_childCountSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ReservationDialog::updateAvailableRooms);
    connect(m_customerDocumentType, &QComboBox::currentIndexChanged, this, &ReservationDialog::updateIdPlaceholder);
    connect(m_customerIdCountry, &QComboBox::currentIndexChanged, this, &ReservationDialog::updateIdPlaceholder);
    connect(m_customerPhoneCode, &QComboBox::currentIndexChanged, this, [this]() { updatePhonePlaceholder(); normalizePhoneInput(); });
    connect(m_customerPhoneLocalEdit, &QLineEdit::textChanged, this, &ReservationDialog::normalizePhoneInput);
    connect(m_existingCustomerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ReservationDialog::applyExistingCustomerSelection);
    connect(m_existingCustomerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this, existingCustomerMode, newCustomerMode](int index) {
        const bool hasExistingSelection = !m_existingCustomerCombo->itemData(index).toString().isEmpty();
        existingCustomerMode->setChecked(hasExistingSelection);
        newCustomerMode->setChecked(!m_existingCustomerMode && !hasExistingSelection);
    });
    // Modified: Make the new-versus-existing customer branch explicit without inserting another modal window before booking.
    connect(existingCustomerMode, &QPushButton::clicked, this, [this]() {
        // Modified: Keep profile fields read-only until staff selects an existing customer instead of accidentally editing a new customer record.
        m_existingCustomerMode = true;
        m_selectedCustomerId.clear();
        showCustomerMode(true);
        setCustomerFieldsEnabled(false);
        if (m_selectedCustomerProfileLabel) {
            m_selectedCustomerProfileLabel->setText("Select a customer to show the saved identity and phone details here.");
        }
        m_existingCustomerCombo->setEditText(QString());
        showValidationMessage("Search for and select an existing customer. Their saved profile will then be shown here.");
        m_existingCustomerCombo->setFocus();
        m_existingCustomerCombo->showPopup();
    });
    connect(newCustomerMode, &QPushButton::clicked, this, [this]() {
        m_existingCustomerMode = false;
        showCustomerMode(false);
        m_existingCustomerCombo->setCurrentIndex(0);
        m_existingCustomerCombo->setEditText(QString());
        setCustomerFieldsEnabled(true);
        m_customerIdEdit->setFocus();
    });
    connect(m_existingCustomerCombo->lineEdit(), &QLineEdit::textEdited, this, [this](const QString& text) {
        if (!m_selectedCustomerId.isEmpty()) {
            // Modified: Returning to search never unlocks a selected profile; staff must explicitly choose New customer to enter new details.
            m_selectedCustomerId.clear();
            m_existingCustomerMode = true;
            setCustomerFieldsEnabled(false);
        }
        filterExistingCustomerPicker(text);
    });
    connect(m_roomCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        confirmPendingRoom();
    });
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(dialogClose, &QPushButton::clicked, this, &QDialog::reject);
    connect(saveBtn, &QPushButton::clicked, this, &ReservationDialog::onAccept);

    const auto clearValidationFeedback = [this]() {
        if (m_validationLabel) {
            m_validationLabel->setText(QString());
            m_validationLabel->setProperty("active", false);
            m_validationLabel->style()->unpolish(m_validationLabel);
            m_validationLabel->style()->polish(m_validationLabel);
        }
    };
    connect(m_customerIdEdit, &QLineEdit::textEdited, this, clearValidationFeedback);
    connect(m_customerNameEdit, &QLineEdit::textEdited, this, clearValidationFeedback);
    connect(m_customerPhoneLocalEdit, &QLineEdit::textEdited, this, clearValidationFeedback);
    connect(m_adultCountSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, clearValidationFeedback);
    connect(m_childCountSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, clearValidationFeedback);
    disableNonMoneyWheelChanges(this);
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

void ReservationDialog::filterExistingCustomerPicker(const QString& searchText)
{
    if (!m_existingCustomerCombo || !m_manager) {
        return;
    }

    // Modified: Support multi-term customer search (for example, "Nguyen + 098") across document number, name, and phone.
    const QStringList terms = searchText.split('+', Qt::SkipEmptyParts);
    QSignalBlocker blocker(m_existingCustomerCombo);
    m_existingCustomerCombo->clear();
    m_existingCustomerCombo->addItem("New customer — enter details manually", QString());

    for (const auto& customer : m_manager->getCustomers()) {
        if (!customer || customer->isArchived()) {
            continue;
        }
        const QString documentNumber = QString::fromStdString(customer->getDocumentNumber());
        const QString name = QString::fromStdString(customer->getName());
        const QString phone = QString::fromStdString(customer->getPhoneNumber());
        const QString searchable = QString("%1 %2 %3").arg(documentNumber, name, phone);
        const bool matches = std::all_of(terms.cbegin(), terms.cend(), [&searchable](const QString& term) {
            return searchable.contains(term.trimmed(), Qt::CaseInsensitive);
        });
        if (matches) {
            m_existingCustomerCombo->addItem(
                QString("%1 — %2 — %3").arg(documentNumber, name, phone),
                QString::fromStdString(customer->getCustomerId()));
        }
    }
    m_existingCustomerCombo->setEditText(searchText);
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

void ReservationDialog::showCustomerMode(bool existingCustomer)
{
    if (m_customerDetailsStack) {
        m_customerDetailsStack->setCurrentIndex(existingCustomer ? 0 : 1);
    }
}

void ReservationDialog::applyExistingCustomerSelection(int index)
{
    const QString selectedId = m_existingCustomerCombo->itemData(index).toString();
    if (selectedId.isEmpty()) {
        // Modified: Keep the customer profile disabled while Existing customer mode has no selected record.
        m_selectedCustomerId.clear();
        setCustomerFieldsEnabled(!m_existingCustomerMode);
        if (!m_existingCustomerMode) {
            m_customerDocumentType->setCurrentIndex(0);
            m_customerIdCountry->setCurrentIndex(0);
            m_customerIdEdit->clear();
            m_customerNameEdit->clear();
            m_customerPhoneCode->setCurrentIndex(0);
            m_customerPhoneLocalEdit->clear();
        }
        if (m_selectedCustomerProfileLabel) {
            m_selectedCustomerProfileLabel->setText("Select a customer to show the saved identity and phone details here.");
        }
        return;
    }
    if (!m_manager) {
        return;
    }

    const auto customer = m_manager->findCustomerById(selectedId.toStdString());
    if (!customer || customer->isArchived()) {
        showValidationMessage("The selected customer is no longer available. Choose another customer or enter a new customer.");
        m_existingCustomerCombo->setCurrentIndex(0);
        m_selectedCustomerId.clear();
        setCustomerFieldsEnabled(true);
        return;
    }

    m_selectedCustomerId = selectedId;
    m_existingCustomerMode = true;
    showCustomerMode(true);
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
    if (m_selectedCustomerProfileLabel) {
        m_selectedCustomerProfileLabel->setText(QString("<b>%1</b><br>%2 · %3")
            .arg(QString::fromStdString(customer->getName()).toHtmlEscaped(),
                 QString::fromStdString(customer->getDocumentNumber()).toHtmlEscaped(),
                 QString::fromStdString(customer->getPhoneNumber()).toHtmlEscaped()));
        m_selectedCustomerProfileLabel->show();
    }
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

void ReservationDialog::updateScheduleSummary()
{
    if (!m_scheduleSummary) {
        return;
    }
    const QDateTime start = QDateTime::fromString(getPlannedCheckInAt(), Qt::ISODate);
    const QDateTime end = QDateTime::fromString(getPlannedCheckOutAt(), Qt::ISODate);
    if (!start.isValid() || !end.isValid()) {
        m_scheduleSummary->setText("Choose a valid check-in and check-out time.");
        return;
    }
    m_scheduleSummary->setText(QString("%1 → %2")
        .arg(start.toString("dd MMM yyyy, HH:mm"), end.toString("dd MMM yyyy, HH:mm")));
}

void ReservationDialog::showValidationMessage(const QString& message)
{
    if (!m_validationLabel) {
        return;
    }
    // Modified: Keep form corrections inside the reservation window so staff are not forced through stacked modal warnings.
    m_validationLabel->setText(QString("Please review: %1").arg(message));
    m_validationLabel->setProperty("active", true);
    m_validationLabel->style()->unpolish(m_validationLabel);
    m_validationLabel->style()->polish(m_validationLabel);
}

void ReservationDialog::openSchedulePicker()
{
    const QDateTime initialCheckIn = QDateTime(m_checkInDateEdit->date(), m_checkInTimeEdit->time());
    const QDateTime initialCheckOut = QDateTime(m_checkOutDateEdit->date(), m_checkOutTimeEdit->time());
    const auto availabilityPredicate = [this](const QDateTime& start, const QDateTime& end) {
        if (!m_manager) {
            return false;
        }
        std::string availabilityError;
        const auto availableRooms = m_manager->getAvailableRoomsForPeriod(
            start.toString(Qt::ISODate).toStdString(), end.toString(Qt::ISODate).toStdString(),
            availabilityError, m_editingBookingId);
        const int guests = m_adultCountSpin->value() + m_childCountSpin->value();
        return std::any_of(availableRooms.begin(), availableRooms.end(), [guests](const std::shared_ptr<Room>& room) {
            return room && guests <= room->getMaximumGuests();
        });
    };

    SchedulePickerDialog picker(initialCheckIn, initialCheckOut, availabilityPredicate, this);
    if (picker.exec() != QDialog::Accepted) {
        return;
    }

    const QDateTime checkIn = picker.selectedCheckIn();
    const QDateTime checkOut = picker.selectedCheckOut();
    m_checkInDateEdit->setDate(checkIn.date());
    m_checkOutDateEdit->setDate(checkOut.date());
    m_checkInTimeEdit->setTime(checkIn.time());
    m_checkOutTimeEdit->setTime(checkOut.time());
    updateScheduleSummary();
    updateAvailableRooms();

    // Modified: Delegate booking scheduling to the shared date-and-hour picker so create and edit flows cannot drift apart.
    return;
}

void ReservationDialog::updateAvailableRooms() {
    const QString previousPendingRoom = m_roomCombo->currentData().toString();
    const QString previouslyConfirmedRoom = m_confirmedRoomNumber;
    m_roomCombo->clear();
    if (!m_manager) return;

    const QString checkInAt = getPlannedCheckInAt();
    const QString checkOutAt = getPlannedCheckOutAt();
    const QDateTime start = QDateTime::fromString(checkInAt, Qt::ISODate);
    const QDateTime end = QDateTime::fromString(checkOutAt, Qt::ISODate);
    if (!start.isValid() || !end.isValid() || end < start.addSecs(60 * 60)) {
        return;
    }

    std::string availabilityError;
    const auto availableRooms = m_manager->getAvailableRoomsForPeriod(
        checkInAt.toStdString(), checkOutAt.toStdString(), availabilityError, m_editingBookingId);
    for (const auto& room : availableRooms) {
        if (m_adultCountSpin->value() + m_childCountSpin->value() > room->getMaximumGuests()) {
            continue;
        }
        // Modified: Render the centrally computed availability list instead of recalculating booking conflicts per room.
        const std::string roomNum = room->getRoomNumber();
        const std::string label = roomNum + " (" + room->getRoomTypeName() + ")";
        m_roomCombo->addItem(QString::fromStdString(label), QString::fromStdString(roomNum));
    }
    int restoreIndex = m_roomCombo->findData(previousPendingRoom);
    if (restoreIndex < 0) {
        restoreIndex = m_roomCombo->findData(previouslyConfirmedRoom);
    }
    if (restoreIndex >= 0) {
        m_roomCombo->setCurrentIndex(restoreIndex);
    }
    if (m_roomCombo->findData(m_confirmedRoomNumber) < 0) {
        m_confirmedRoomNumber.clear();
    }
    updateRoomReview();
}

void ReservationDialog::updateRoomReview()
{
    if (!m_roomReviewLabel || !m_manager || m_roomCombo->currentIndex() < 0) {
        if (m_roomReviewLabel) {
            m_roomReviewLabel->setText("Choose a room to review its capacity, bed setup, amenities, and hourly rate.");
        }
        return;
    }
    const auto room = m_manager->findRoomByNumber(m_roomCombo->currentData().toString().toStdString());
    if (!room) {
        m_roomReviewLabel->setText("This room is no longer available. Choose another room.");
        return;
    }
    const QString pendingRoom = QString::fromStdString(room->getRoomNumber()).toHtmlEscaped();
    const QString selectionState = pendingRoom == m_confirmedRoomNumber
        ? "<b style='color:#047857;'>Selected for this reservation</b>"
        : "Select this room from the list to use it for this reservation.";
    const QString amenities = QString::fromStdString(room->getAmenities()).trimmed().isEmpty()
        ? "Not listed" : QString::fromStdString(room->getAmenities()).toHtmlEscaped();
    const QString roomType = QString::fromStdString(room->getRoomTypeName()).toHtmlEscaped();
    const QString bedType = QString::fromStdString(room->getBedType()).trimmed().isEmpty()
        ? "Not listed" : QString::fromStdString(room->getBedType()).toHtmlEscaped();
    const QString rate = QLocale(QLocale::English).toString(static_cast<qlonglong>(room->getBasePrice()));
    // Modified: Keep room comparison in the booking dialog so staff can verify commercial and accommodation details without opening a second modal window.
    m_roomReviewLabel->setText(QString(
        "%1<br><b>Room %2 · %3</b><br>Capacity: %4 guests · Bed: %5<br>Hourly rate: %6 VND<br>Amenities: %7")
        .arg(selectionState, pendingRoom, roomType)
        .arg(room->getMaximumGuests())
        .arg(bedType)
        .arg(rate, amenities));
}

void ReservationDialog::confirmPendingRoom()
{
    if (m_roomCombo->currentIndex() < 0) {
        // Modified: A schedule refresh may temporarily empty the one-click room list; keep that passive state out of the validation channel.
        m_confirmedRoomNumber.clear();
        updateRoomReview();
        return;
    }
    // Modified: A room selection is committed in one click, while the review panel immediately confirms the exact selected room and rate.
    m_confirmedRoomNumber = m_roomCombo->currentData().toString();
    updateRoomReview();
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

    if (m_existingCustomerMode && !usingExistingCustomer) {
        const QString searchText = m_existingCustomerCombo->currentText().trimmed();
        if (!searchText.isEmpty() && searchText != "New customer — enter details manually") {
            // Modified: Ask only when staff tries to save an unsuccessful existing-customer search, avoiding disruptive prompts while they type.
            CustomConfirmDialog createCustomerPrompt(
                "Customer not found",
                "No existing customer matches this search. Is this a new customer?",
                false, this, "Create new customer", "Keep searching");
            if (createCustomerPrompt.exec() == QDialog::Accepted && createCustomerPrompt.isConfirmed()) {
                m_existingCustomerMode = false;
                showCustomerMode(false);
                populateExistingCustomerPicker();
                m_existingCustomerCombo->setCurrentIndex(0);
                m_existingCustomerCombo->setEditText(QString());
                setCustomerFieldsEnabled(true);
                showValidationMessage("Enter the new customer's identity and contact details, then save the reservation.");
                m_customerIdEdit->setFocus();
            }
            return;
        }
        showValidationMessage("Select a customer from the search results, or choose New customer to enter a new profile.");
        return;
    }

    if (!usingExistingCustomer && m_editingBookingId.empty() && !isValidDocumentNumber(documentType, idRule.key, customerId)) {
        showValidationMessage(QString("%1 for %2 must be %3.").arg(documentType, idRule.name, documentNumberHint(documentType, idRule.key)));
        return;
    }

    if (!usingExistingCustomer && m_editingBookingId.empty() && m_manager) {
        if (const auto existingCustomer = m_manager->findCustomerById(customerId.toStdString())) {
            if (existingCustomer->isArchived()) {
                showValidationMessage("This identity belongs to an archived customer record. Restore the customer in Customer Management before booking.");
                return;
            }
            // Modified: Reuse the authoritative stored profile when a new-customer entry matches an existing document identity, preventing a duplicate guest record.
            m_existingCustomerMode = true;
            showCustomerMode(true);
            populateExistingCustomerPicker();
            const int existingIndex = m_existingCustomerCombo->findData(
                QString::fromStdString(existingCustomer->getCustomerId()));
            if (existingIndex >= 0) {
                m_existingCustomerCombo->setCurrentIndex(existingIndex);
                showValidationMessage("An existing customer record was found for this identity. The saved profile is now selected.");
                return;
            }
        }
    }

    if (!usingExistingCustomer && !HotelManager::isValidCustomerNameFormat(customerName.toStdString())) {
        showValidationMessage("Enter a legal name using letters, spaces, apostrophes, hyphens, or initials.");
        return;
    }

    static const QRegularExpression digitsPattern(QStringLiteral(R"(^\d+$)"));
    if (!usingExistingCustomer && !digitsPattern.match(phoneLocal).hasMatch()) {
        showValidationMessage("The phone number can contain digits only.");
        return;
    }

    if (!usingExistingCustomer && (phoneLocal.size() < phoneRule.phoneMinDigits || phoneLocal.size() > phoneRule.phoneMaxDigits)) {
        showValidationMessage(QString("The phone number for %1 must be %2.").arg(phoneRule.name, phoneRule.phoneHint));
        return;
    }

    if (!usingExistingCustomer && !HotelManager::isValidPhoneNumberFormat(fullPhone.toStdString())) {
        showValidationMessage("The phone number format is invalid.");
        return;
    }

    if (!usingExistingCustomer && m_editingBookingId.empty() && m_manager) {
        const auto matchingPhoneCustomer = std::find_if(
            m_manager->getCustomers().cbegin(), m_manager->getCustomers().cend(),
            [&fullPhone](const std::shared_ptr<Customer>& customer) {
                return customer && customer->getPhoneNumber() == fullPhone.toStdString();
            });
        if (matchingPhoneCustomer != m_manager->getCustomers().cend()) {
            if ((*matchingPhoneCustomer)->isArchived()) {
                showValidationMessage("This phone number belongs to an archived customer record. Restore the customer in Customer Management before booking.");
                return;
            }
            // Modified: Route a new-customer entry with an existing phone to the canonical customer picker instead of asking staff to invent another phone number.
            m_existingCustomerMode = true;
            showCustomerMode(true);
            populateExistingCustomerPicker();
            const int existingIndex = m_existingCustomerCombo->findData(
                QString::fromStdString((*matchingPhoneCustomer)->getCustomerId()));
            if (existingIndex >= 0) {
                m_existingCustomerCombo->setCurrentIndex(existingIndex);
                showValidationMessage("An existing customer record was found for this phone number. The saved profile is now selected.");
                return;
            }
        }
    }

    if (m_confirmedRoomNumber.isEmpty()) {
        showValidationMessage("Select an available room before creating the reservation.");
        return;
    }

    if (m_roomCombo->currentData().toString() != m_confirmedRoomNumber) {
        showValidationMessage("The room list has changed. Select the intended room again before saving.");
        return;
    }

    if (m_roomCombo->currentIndex() < 0) {
        showValidationMessage("No room is available for this schedule. Choose another date, hour, or guest count.");
        return;
    }

    const QDateTime plannedCheckIn = QDateTime::fromString(getPlannedCheckInAt(), Qt::ISODate);
    const QDateTime plannedCheckOut = QDateTime::fromString(getPlannedCheckOutAt(), Qt::ISODate);
    if (!plannedCheckIn.isValid() || !plannedCheckOut.isValid() || plannedCheckOut < plannedCheckIn.addSecs(60 * 60)) {
        showValidationMessage("A reservation must be at least one hour long.");
        return;
    }

    if (m_adultCountSpin->value() + m_childCountSpin->value() <= 0) {
        showValidationMessage("A reservation must include at least one guest.");
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
    return m_confirmedRoomNumber;
}

QString ReservationDialog::getCheckInDate() const {
    return m_checkInDateEdit->date().toString("yyyy-MM-dd");
}

QString ReservationDialog::getCheckOutDate() const {
    return m_checkOutDateEdit->date().toString("yyyy-MM-dd");
}

QString ReservationDialog::getPlannedCheckInAt() const {
    return QDateTime(m_checkInDateEdit->date(), m_checkInTimeEdit->time()).toString(Qt::ISODate);
}

QString ReservationDialog::getPlannedCheckOutAt() const {
    return QDateTime(m_checkOutDateEdit->date(), m_checkOutTimeEdit->time()).toString(Qt::ISODate);
}

int ReservationDialog::getAdultCount() const {
    return m_adultCountSpin->value();
}

int ReservationDialog::getChildCount() const {
    return m_childCountSpin->value();
}

bool ReservationDialog::usesExistingCustomer() const
{
    return !m_selectedCustomerId.isEmpty();
}

void ReservationDialog::setEditBooking(const std::string& bookingId) {
    m_editingBookingId = bookingId;
    if (!m_manager) return;

    auto booking = m_manager->findBookingById(bookingId);
    if (!booking) return;

    setWindowTitle("Edit Reservation");
    if (auto* title = findChild<QLabel*>("reservationDialogTitle")) {
        title->setText("Edit reservation");
    }
    if (auto* subtitle = findChild<QLabel*>("reservationDialogSubtitle")) {
        subtitle->setText("Review the selected guest and adjust the planned schedule or room.");
    }
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
    // Modified: Customer identity and contact fields stay read-only while editing a reservation; customer changes belong to Customer Management.
    setCustomerFieldsEnabled(false);
    m_existingCustomerCombo->setEnabled(false);

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
    const QDateTime plannedInAt = QDateTime::fromString(QString::fromStdString(booking->getPlannedCheckInAt()), Qt::ISODate);
    const QDateTime plannedOutAt = QDateTime::fromString(QString::fromStdString(booking->getPlannedCheckOutAt()), Qt::ISODate);
    if (plannedInAt.isValid()) {
        m_checkInTimeEdit->setTime(plannedInAt.time());
    }
    if (plannedOutAt.isValid()) {
        m_checkOutTimeEdit->setTime(plannedOutAt.time());
    }
    m_adultCountSpin->setValue(booking->getAdultCount());
    m_childCountSpin->setValue(booking->getChildCount());
    if (m_manager->getBookingState(*booking) == BookingState::ACTIVE) {
        // Modified: Active stays retain their original arrival date; checkout remains the explicit completion action.
        m_checkInDateEdit->setEnabled(false);
        m_scheduleButton->setEnabled(false);
    }

    updateScheduleSummary();
    updateAvailableRooms();

    std::string currentRoom = booking->getRoom()->getRoomNumber();
    int idx = m_roomCombo->findData(QString::fromStdString(currentRoom));
    if (idx >= 0) {
        m_roomCombo->setCurrentIndex(idx);
        confirmPendingRoom();
    }
}

bool ReservationDialog::selectRoom(const std::string& roomNumber) {
    updateAvailableRooms();
    const int roomIndex = m_roomCombo->findData(QString::fromStdString(roomNumber));
    if (roomIndex < 0) {
        return false;
    }

    m_roomCombo->setCurrentIndex(roomIndex);
    confirmPendingRoom();
    return true;
}

void ReservationDialog::setInitialSchedule(const QDate& checkIn,
                                           const QDate& checkOut,
                                           int adults,
                                           int children)
{
    if (!checkIn.isValid() || !checkOut.isValid() || checkOut <= checkIn) {
        return;
    }

    setInitialScheduleAt(QDateTime(checkIn, QTime(0, 0)), QDateTime(checkOut, QTime(0, 0)), adults, children);
}

void ReservationDialog::setInitialScheduleAt(const QDateTime& checkIn,
                                             const QDateTime& checkOut,
                                             int adults,
                                             int children)
{
    if (!checkIn.isValid() || !checkOut.isValid() || checkOut < checkIn.addSecs(60 * 60)) {
        return;
    }

    // Modified: Preserve the exact Room Status hour selection when its availability request opens Reservation.
    QDateTime initialStart(checkIn.date(), QTime(checkIn.time().hour(), 0));
    const QDateTime now = QDateTime::currentDateTime();
    if (initialStart < now) {
        initialStart = now.addSecs(60 * 60);
        initialStart.setTime(QTime(initialStart.time().hour(), 0));
        if (initialStart <= now) {
            initialStart = initialStart.addSecs(60 * 60);
        }
    }
    QDateTime initialEnd(checkOut.date(), QTime(checkOut.time().hour(), 0));
    if (initialEnd < initialStart.addSecs(60 * 60)) {
        initialEnd = initialStart.addSecs(60 * 60);
    }
    // Modified: A Room Status request for today starts at the next whole hour, never at an already elapsed midnight timestamp.
    m_checkInDateEdit->setDate(initialStart.date());
    m_checkOutDateEdit->setDate(initialEnd.date());
    m_checkInTimeEdit->setTime(initialStart.time());
    m_checkOutTimeEdit->setTime(initialEnd.time());
    m_adultCountSpin->setValue(std::max(1, adults));
    m_childCountSpin->setValue(std::max(0, children));
    updateScheduleSummary();
    updateAvailableRooms();
}
