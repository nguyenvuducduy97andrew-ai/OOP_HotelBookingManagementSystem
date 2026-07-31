#include "RoomDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QLocale>
#include <QDate>
#include <QColor>
#include <QPalette>

RoomDialog::RoomDialog(QWidget *parent)
    : QDialog(parent), m_isEditMode(false) {
    setupUI();
    setWindowTitle("Add New Room");
    onTypeChanged(0); // Standard by default
}

RoomDialog::RoomDialog(const QString& roomNum, double basePrice, RoomType type, double extraFee, bool isAvailable, QWidget *parent)
    : QDialog(parent), m_isEditMode(true), m_originalRoomNum(roomNum) {
    setupUI();
    setWindowTitle("Edit Room");

    m_roomNumberEdit->setText(roomNum);
    m_roomNumberEdit->setEnabled(false); // Can't change room number during edit to prevent PK change
    m_basePriceSpin->setValue(basePrice);

    int index = 0;
    if (type == RoomType::Deluxe) index = 1;
    else if (type == RoomType::Suite) index = 2;
    m_typeCombo->setCurrentIndex(index);
    m_typeCombo->setEnabled(false); // Can't change type during edit to preserve subclass runtime type

    m_availabilityCombo->setCurrentIndex(isAvailable ? 0 : 1);

    onTypeChanged(index);
    if (type == RoomType::Deluxe || type == RoomType::Suite) {
        m_extraFeeSpin->setValue(extraFee);
    }
}

void setupDialogStyle(QDialog* dialog) {
    dialog->setStyleSheet(R"(
        QDialog {
            background-color: #FFFFFF;
        }
        QLabel {
            font-size: 13px;
            color: #2B3674;
            font-weight: 600;
        }
        QLineEdit, QComboBox, QDoubleSpinBox, QDateEdit, QTextEdit {
            background-color: #F4F7FE;
            border: 1px solid #E9EDF7;
            border-radius: 8px;
            padding: 6px 12px;
            font-size: 13px;
            color: #2B3674;
        }
        QTextEdit {
            selection-background-color: #005BFE;
            selection-color: #FFFFFF;
        }
        QLineEdit:focus, QComboBox:focus, QDoubleSpinBox:focus, QDateEdit:focus, QTextEdit:focus {
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
        QPushButton#btnCancelMaintenance {
            background-color: #FEE2E2;
            color: #B91C1C;
            font-weight: 600;
            border: 1px solid #FCA5A5;
            border-radius: 8px;
            padding: 8px 16px;
            font-size: 13px;
        }
        QPushButton#btnCancelMaintenance:hover {
            background-color: #FECACA;
        }
        QPushButton#btnCancelMaintenance:disabled {
            background-color: #F1F5F9;
            color: #64748B;
            border-color: #E2E8F0;
        }
        QPushButton#btnConfirmMaintenance {
            background-color: #ECFDF5;
            color: #047857;
            font-weight: 600;
            border: 1px solid #A7F3D0;
            border-radius: 8px;
            padding: 8px 16px;
            font-size: 13px;
        }
        QPushButton#btnConfirmMaintenance:hover { background-color: #D1FAE5; }
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

void RoomDialog::setupUI() {
    setupDialogStyle(this);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    auto* formLayout = new QFormLayout();
    formLayout->setSpacing(12);

    m_roomNumberEdit = new QLineEdit(this);
    m_roomNumberEdit->setPlaceholderText("Example: 101, 302");
    formLayout->addRow("Room Number:", m_roomNumberEdit);

    const QLocale moneyLocale(QLocale::Vietnamese, QLocale::Vietnam);

    m_basePriceSpin = new QDoubleSpinBox(this);
    m_basePriceSpin->setRange(0, 100000000);
    m_basePriceSpin->setSingleStep(50000);
    m_basePriceSpin->setSuffix(" VND");
    m_basePriceSpin->setDecimals(0);
    m_basePriceSpin->setLocale(moneyLocale);
    m_basePriceSpin->setGroupSeparatorShown(true);
    formLayout->addRow("Base Price (per night):", m_basePriceSpin);

    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItems({"Standard", "Deluxe", "Suite"});
    formLayout->addRow("Room Type:", m_typeCombo);

    m_availabilityCombo = new QComboBox(this);
    m_availabilityCombo->addItems({"Available", "Schedule maintenance"});
    formLayout->addRow("Status:", m_availabilityCombo);

    m_maintenanceStartLabel = new QLabel("Maintenance starts:", this);
    m_maintenanceStartDateEdit = new QDateEdit(QDate::currentDate(), this);
    // Modified: Prevent operational maintenance scheduling from creating a past interval.
    m_maintenanceStartDateEdit->setMinimumDate(QDate::currentDate());
    m_maintenanceStartDateEdit->setCalendarPopup(true);
    m_maintenanceStartDateEdit->setDisplayFormat("dd MMM yyyy");
    formLayout->addRow(m_maintenanceStartLabel, m_maintenanceStartDateEdit);

    m_maintenanceEndLabel = new QLabel("Available again on:", this);
    m_maintenanceEndDateEdit = new QDateEdit(QDate::currentDate().addDays(1), this);
    m_maintenanceEndDateEdit->setMinimumDate(QDate::currentDate().addDays(1));
    m_maintenanceEndDateEdit->setCalendarPopup(true);
    m_maintenanceEndDateEdit->setDisplayFormat("dd MMM yyyy");
    formLayout->addRow(m_maintenanceEndLabel, m_maintenanceEndDateEdit);

    m_maintenanceNoteLabel = new QLabel("Maintenance note:", this);
    m_maintenanceNoteEdit = new QTextEdit(this);
    m_maintenanceNoteEdit->setPlaceholderText("Optional reason or work order reference");
    QPalette maintenanceNotePalette = m_maintenanceNoteEdit->palette();
    maintenanceNotePalette.setColor(QPalette::Text, QColor("#2B3674"));
    maintenanceNotePalette.setColor(QPalette::PlaceholderText, QColor("#718096"));
    m_maintenanceNoteEdit->setPalette(maintenanceNotePalette);
    m_maintenanceNoteEdit->setFixedHeight(62);
    formLayout->addRow(m_maintenanceNoteLabel, m_maintenanceNoteEdit);

    m_existingMaintenanceLabel = new QLabel("Scheduled maintenance:", this);
    m_existingMaintenanceCombo = new QComboBox(this);
    formLayout->addRow(m_existingMaintenanceLabel, m_existingMaintenanceCombo);
    m_cancelMaintenanceBtn = new QPushButton("Cancel selected schedule", this);
    m_cancelMaintenanceBtn->setObjectName("btnCancelMaintenance");
    m_existingMaintenanceLabel->setVisible(false);
    m_existingMaintenanceCombo->setVisible(false);
    m_cancelMaintenanceBtn->setVisible(false);
    m_confirmMaintenanceBtn = new QPushButton("Confirm resolved case", this);
    m_confirmMaintenanceBtn->setObjectName("btnConfirmMaintenance");
    m_confirmMaintenanceBtn->setVisible(false);

    m_extraFeeLabel = new QLabel(this);
    m_extraFeeSpin = new QDoubleSpinBox(this);
    m_extraFeeSpin->setRange(0, 100000000);
    m_extraFeeSpin->setSingleStep(10000);
    m_extraFeeSpin->setSuffix(" VND");
    m_extraFeeSpin->setDecimals(0);
    m_extraFeeSpin->setLocale(moneyLocale);
    m_extraFeeSpin->setGroupSeparatorShown(true);
    formLayout->addRow(m_extraFeeLabel, m_extraFeeSpin);

    mainLayout->addLayout(formLayout);

    // Button Row
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    auto* cancelBtn = new QPushButton("Cancel", this);
    cancelBtn->setObjectName("btnCancel");
    auto* saveBtn = new QPushButton("Save", this);
    saveBtn->setObjectName("btnSave");

    btnLayout->addWidget(m_cancelMaintenanceBtn);
    btnLayout->addWidget(m_confirmMaintenanceBtn);
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(saveBtn);
    mainLayout->addLayout(btnLayout);

    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &RoomDialog::onTypeChanged);
    connect(m_availabilityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &RoomDialog::onStatusChanged);
    connect(m_cancelMaintenanceBtn, &QPushButton::clicked, this, &RoomDialog::markSelectedMaintenanceForCancellation);
    connect(m_confirmMaintenanceBtn, &QPushButton::clicked, this, &RoomDialog::markSelectedMaintenanceForConfirmation);
    connect(m_existingMaintenanceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        const bool pendingGuestResponse = m_existingMaintenanceCombo->currentData(Qt::UserRole + 1).toString() == "Awaiting guest response";
        m_confirmMaintenanceBtn->setEnabled(pendingGuestResponse);
    });
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(saveBtn, &QPushButton::clicked, this, &RoomDialog::onAccept);
    onStatusChanged(m_availabilityCombo->currentIndex());
}

void RoomDialog::onTypeChanged(int index) {
    if (index == 0) { // Standard
        m_extraFeeLabel->setVisible(false);
        m_extraFeeSpin->setVisible(false);
    } else if (index == 1) { // Deluxe
        m_extraFeeLabel->setText("Mini Bar Fee:");
        m_extraFeeLabel->setVisible(true);
        m_extraFeeSpin->setVisible(true);
    } else if (index == 2) { // Suite
        m_extraFeeLabel->setText("Premium Service Fee:");
        m_extraFeeLabel->setVisible(true);
        m_extraFeeSpin->setVisible(true);
    }
    adjustSize();
}

void RoomDialog::onAccept() {
    if (m_roomNumberEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Missing information", "Please enter the room number.");
        return;
    }
    if (m_basePriceSpin->value() <= 0) {
        QMessageBox::warning(this, "Invalid value", "Please enter a room price greater than 0.");
        return;
    }
    if (shouldScheduleMaintenance() && m_maintenanceEndDateEdit->date() <= m_maintenanceStartDateEdit->date()) {
        QMessageBox::warning(this, "Invalid maintenance dates", "The date the room becomes available again must be after the maintenance start date.");
        return;
    }
    if ((!m_maintenanceIdToCancel.isEmpty() || !m_maintenanceIdToConfirm.isEmpty()) && shouldScheduleMaintenance()) {
        QMessageBox::warning(this, "Complete one maintenance action", "Save the schedule cancellation first, then create a new maintenance schedule.");
        return;
    }
    accept();
}

QString RoomDialog::getRoomNumber() const {
    return m_roomNumberEdit->text().trimmed();
}

double RoomDialog::getBasePrice() const {
    return m_basePriceSpin->value();
}

RoomType RoomDialog::getRoomType() const {
    int idx = m_typeCombo->currentIndex();
    if (idx == 1) return RoomType::Deluxe;
    if (idx == 2) return RoomType::Suite;
    return RoomType::Standard;
}

double RoomDialog::getExtraFee() const {
    if (m_typeCombo->currentIndex() == 0) return 0.0;
    return m_extraFeeSpin->value();
}

bool RoomDialog::getIsAvailable() const {
    return m_availabilityCombo->currentIndex() == 0;
}

bool RoomDialog::shouldScheduleMaintenance() const {
    return m_availabilityCombo->currentIndex() == 1;
}

QString RoomDialog::getMaintenanceStartDate() const {
    return m_maintenanceStartDateEdit->date().toString(Qt::ISODate);
}

QString RoomDialog::getMaintenanceEndDate() const {
    return m_maintenanceEndDateEdit->date().toString(Qt::ISODate);
}

QString RoomDialog::getMaintenanceNote() const {
    return m_maintenanceNoteEdit->toPlainText().trimmed();
}

void RoomDialog::setExistingMaintenanceSchedules(const std::vector<RoomMaintenance>& schedules,
                                                 const std::vector<MaintenanceGuestNotice>& notices) {
    m_existingMaintenanceCombo->clear();
    m_maintenanceIdToCancel.clear();
    m_maintenanceIdToConfirm.clear();

    for (const RoomMaintenance& maintenance : schedules) {
        int noticeCount = 0;
        for (const auto& notice : notices) {
            if (notice.getMaintenanceId() == maintenance.getMaintenanceId()) {
                ++noticeCount;
            }
        }
        QString label = QString("%1 | %2 to %3")
            .arg(QString::fromStdString(maintenance.getStatus()))
            .arg(QString::fromStdString(maintenance.getStartDate()))
            .arg(QString::fromStdString(maintenance.getEndDate()));
        if (noticeCount > 0) {
            // Modified: render maintenance-notice counts with a real singular or plural booking label.
            label += QString(" | Simulated email logged for %1 %2")
                .arg(noticeCount)
                .arg(noticeCount == 1 ? QStringLiteral("booking") : QStringLiteral("bookings"));
        }
        if (!maintenance.getNote().empty()) {
            label += QString(" — %1").arg(QString::fromStdString(maintenance.getNote()));
        }
        m_existingMaintenanceCombo->addItem(label, QString::fromStdString(maintenance.getMaintenanceId()));
        m_existingMaintenanceCombo->setItemData(m_existingMaintenanceCombo->count() - 1,
                                                QString::fromStdString(maintenance.getStatus()), Qt::UserRole + 1);
    }

    const bool hasSchedules = m_existingMaintenanceCombo->count() > 0;
    m_existingMaintenanceLabel->setText("Scheduled maintenance:");
    m_existingMaintenanceLabel->setVisible(hasSchedules);
    m_existingMaintenanceCombo->setVisible(hasSchedules);
    m_existingMaintenanceCombo->setEnabled(hasSchedules);
    m_cancelMaintenanceBtn->setText("Cancel selected schedule");
    m_cancelMaintenanceBtn->setEnabled(hasSchedules);
    m_cancelMaintenanceBtn->setVisible(hasSchedules);
    const bool pendingGuestResponse = hasSchedules
        && m_existingMaintenanceCombo->currentData(Qt::UserRole + 1).toString() == "Awaiting guest response";
    m_confirmMaintenanceBtn->setEnabled(pendingGuestResponse);
    m_confirmMaintenanceBtn->setVisible(hasSchedules);
}

QString RoomDialog::getMaintenanceIdToCancel() const {
    return m_maintenanceIdToCancel;
}

QString RoomDialog::getMaintenanceIdToConfirm() const { return m_maintenanceIdToConfirm; }

void RoomDialog::markSelectedMaintenanceForCancellation() {
    if (m_existingMaintenanceCombo->currentIndex() < 0) {
        return;
    }

    // Modified: Stage one maintenance cancellation and persist it atomically with the room edit.
    m_maintenanceIdToCancel = m_existingMaintenanceCombo->currentData().toString();
    m_existingMaintenanceLabel->setText("Cancelled");
    m_existingMaintenanceCombo->setEnabled(false);
    m_cancelMaintenanceBtn->setText("Cancellation selected");
    m_cancelMaintenanceBtn->setEnabled(false);
}

void RoomDialog::markSelectedMaintenanceForConfirmation() {
    if (m_existingMaintenanceCombo->currentIndex() < 0) {
        return;
    }

    // Modified: Close immediately after selecting confirmation; the parent workflow rechecks live booking conflicts before persistence.
    m_maintenanceIdToConfirm = m_existingMaintenanceCombo->currentData().toString();
    accept();
}

void RoomDialog::onStatusChanged(int index) {
    const bool scheduleMaintenance = index == 1;
    m_maintenanceStartLabel->setVisible(scheduleMaintenance);
    m_maintenanceStartDateEdit->setVisible(scheduleMaintenance);
    m_maintenanceEndLabel->setVisible(scheduleMaintenance);
    m_maintenanceEndDateEdit->setVisible(scheduleMaintenance);
    m_maintenanceNoteLabel->setVisible(scheduleMaintenance);
    m_maintenanceNoteEdit->setVisible(scheduleMaintenance);
    adjustSize();
}
