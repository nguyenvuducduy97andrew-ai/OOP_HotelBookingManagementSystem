#include "RoomDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QMessageBox>

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
        QLineEdit, QComboBox, QDoubleSpinBox {
            background-color: #F4F7FE;
            border: 1px solid #E9EDF7;
            border-radius: 8px;
            padding: 6px 12px;
            font-size: 13px;
            color: #2B3674;
        }
        QLineEdit:focus, QComboBox:focus, QDoubleSpinBox:focus {
            border: 1px solid #005BFE;
        }
        QComboBox QAbstractItemView {
            background-color: #FFFFFF;
            color: #2B3674;
            selection-background-color: #005BFE;
            selection-color: #FFFFFF;
            border: 1px solid #E9EDF7;
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

    m_basePriceSpin = new QDoubleSpinBox(this);
    m_basePriceSpin->setRange(0, 100000000);
    m_basePriceSpin->setSingleStep(50000);
    m_basePriceSpin->setSuffix(" đ");
    m_basePriceSpin->setDecimals(0);
    formLayout->addRow("Base Price (per night):", m_basePriceSpin);

    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItems({"Standard", "Deluxe", "Suite"});
    formLayout->addRow("Room Type:", m_typeCombo);

    m_availabilityCombo = new QComboBox(this);
    m_availabilityCombo->addItems({"Available", "Maintenance"});
    formLayout->addRow("Status:", m_availabilityCombo);

    m_extraFeeLabel = new QLabel(this);
    m_extraFeeSpin = new QDoubleSpinBox(this);
    m_extraFeeSpin->setRange(0, 100000000);
    m_extraFeeSpin->setSingleStep(10000);
    m_extraFeeSpin->setSuffix(" đ");
    m_extraFeeSpin->setDecimals(0);
    formLayout->addRow(m_extraFeeLabel, m_extraFeeSpin);

    mainLayout->addLayout(formLayout);

    // Button Row
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    auto* cancelBtn = new QPushButton("Cancel", this);
    cancelBtn->setObjectName("btnCancel");
    auto* saveBtn = new QPushButton("Save", this);
    saveBtn->setObjectName("btnSave");

    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(saveBtn);
    mainLayout->addLayout(btnLayout);

    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &RoomDialog::onTypeChanged);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(saveBtn, &QPushButton::clicked, this, &RoomDialog::onAccept);
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
