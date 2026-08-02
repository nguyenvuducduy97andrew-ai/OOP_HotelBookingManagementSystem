#include "RoomInfoDialog.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>

RoomInfoDialog::RoomInfoDialog(std::shared_ptr<Room> room, QWidget *parent)
    : QDialog(parent), m_room(std::move(room)) {
    
    setupUI();
    setupStyle();
    populateData();
    
    setMinimumWidth(520);
    setWindowTitle("Room Information");
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
}

QString RoomInfoDialog::formatMoney(double amount) {
    QString str = QString::number(static_cast<long long>(amount));
    int insertPosition = str.length() - 3;
    while (insertPosition > 0) {
        str.insert(insertPosition, ",");
        insertPosition -= 3;
    }
    return str;
}

void RoomInfoDialog::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    
    auto* bgFrame = new QFrame(this);
    bgFrame->setObjectName("bgFrame");
    auto* frameLayout = new QVBoxLayout(bgFrame);
    frameLayout->setContentsMargins(24, 24, 24, 24);
    frameLayout->setSpacing(16);
    
    // Header row
    auto* headerLayout = new QHBoxLayout();
    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName("titleLabel");
    
    m_statusBadge = new QLabel("Available", this);
    m_statusBadge->setObjectName("statusBadge");
    
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addWidget(m_statusBadge);
    headerLayout->addStretch();
    
    // Image placeholder
    m_imageLabel = new QLabel(this);
    m_imageLabel->setObjectName("imageLabel");
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setFixedHeight(180);
    
    // Specs row
    auto* specsLayout = new QHBoxLayout();
    m_sizeLabel = new QLabel(this);
    m_sizeLabel->setObjectName("specLabel");
    m_bedLabel = new QLabel(this);
    m_bedLabel->setObjectName("specLabel");
    m_guestLabel = new QLabel(this);
    m_guestLabel->setObjectName("specLabel");
    
    specsLayout->addWidget(m_sizeLabel);
    specsLayout->addStretch();
    specsLayout->addWidget(m_bedLabel);
    specsLayout->addStretch();
    specsLayout->addWidget(m_guestLabel);
    
    // Extra fee
    m_extraFeeLabel = new QLabel(this);
    m_extraFeeLabel->setObjectName("extraFeeLabel");
    m_extraFeeLabel->setAlignment(Qt::AlignCenter);
    
    // Description
    auto* descTitle = new QLabel("Room Description", this);
    descTitle->setObjectName("sectionTitle");
    m_descLabel = new QLabel(this);
    m_descLabel->setObjectName("descLabel");
    m_descLabel->setWordWrap(true);
    
    // Amenities
    auto* amTitle = new QLabel("Amenities", this);
    amTitle->setObjectName("sectionTitle");
    auto* amLabel = new QLabel("✓ Free Wi-Fi   ·   ✓ Smart TV   ·   ✓ Air Conditioning   ·   ✓ Private bathroom", this);
    amLabel->setObjectName("amLabel");
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    m_btnCancel = new QPushButton("Cancel", this);
    m_btnCancel->setObjectName("btnCancel");
    m_btnBooking = new QPushButton("Booking", this);
    m_btnBooking->setObjectName("btnBooking");
    
    btnLayout->addWidget(m_btnCancel);
    btnLayout->addWidget(m_btnBooking);
    
    // Assemble
    frameLayout->addLayout(headerLayout);
    frameLayout->addWidget(m_imageLabel);
    frameLayout->addSpacing(16);
    frameLayout->addLayout(specsLayout);
    frameLayout->addSpacing(12);
    frameLayout->addWidget(m_extraFeeLabel);
    frameLayout->addSpacing(10);
    frameLayout->addWidget(descTitle);
    frameLayout->addWidget(m_descLabel);
    frameLayout->addSpacing(10);
    frameLayout->addWidget(amTitle);
    frameLayout->addWidget(amLabel);
    frameLayout->addStretch();
    frameLayout->addLayout(btnLayout);
    
    mainLayout->addWidget(bgFrame);
    
    connect(m_btnCancel, &QPushButton::clicked, this, &RoomInfoDialog::onCancelClicked);
    connect(m_btnBooking, &QPushButton::clicked, this, &RoomInfoDialog::onBookingClicked);
}

void RoomInfoDialog::setupStyle() {
    setStyleSheet(R"(
        QFrame#bgFrame {
            background-color: #FFFFFF;
            border-radius: 20px;
            border: 1px solid #E9EDF7;
        }
        QLabel#titleLabel {
            font-size: 24px;
            font-weight: 800;
            color: #1B2559;
        }
        QLabel#statusBadge {
            background-color: #E2FBE8;
            color: #05A660;
            padding: 4px 12px;
            border-radius: 12px;
            font-weight: 700;
            font-size: 13px;
        }
        QLabel#specLabel {
            font-size: 15px;
            font-weight: 700;
            color: #1B2559;
        }
        QLabel#extraFeeLabel {
            background-color: #F4F7FE;
            color: #005BFE;
            padding: 10px;
            border-radius: 8px;
            font-weight: 700;
            font-size: 14px;
            margin-top: 10px;
        }
        QLabel#sectionTitle {
            font-size: 16px;
            font-weight: 700;
            color: #1B2559;
        }
        QLabel#descLabel {
            font-size: 14px;
            color: #A3AED0;
            line-height: 1.5;
        }
        QLabel#amLabel {
            font-size: 14px;
            color: #A3AED0;
            font-weight: 500;
        }
        QPushButton#btnCancel {
            background-color: #F4F7FE;
            color: #2B3674;
            font-weight: 700;
            border-radius: 12px;
            padding: 12px 20px;
            font-size: 14px;
            border: none;
        }
        QPushButton#btnCancel:hover {
            background-color: #E9EDF7;
        }
        QPushButton#btnBooking {
            background-color: #005BFE;
            color: #FFFFFF;
            font-weight: 700;
            border-radius: 12px;
            padding: 12px 20px;
            font-size: 14px;
            border: none;
        }
        QPushButton#btnBooking:hover {
            background-color: #2B7BFF;
        }
    )");
}

void RoomInfoDialog::populateData() {
    if (!m_room) return;
    
    m_titleLabel->setText(QString("Room %1").arg(QString::fromStdString(m_room->getRoomNumber())));
    m_descLabel->setText("This room is currently available and can be booked for your selected dates.");
    
    const std::string typeName = m_room->getRoomTypeName();
    if (typeName == "Standard") {
        m_sizeLabel->setText("📏 25m²");
        m_bedLabel->setText("🛏 Queen Bed");
        m_guestLabel->setText(QString("👤 %1 Guests").arg(m_room->getMaximumGuests()));
        m_extraFeeLabel->setVisible(false);
        m_imageLabel->setText("🏨 Standard Room Image Placeholder");
        m_imageLabel->setStyleSheet("background-color: #F4F7FE; border-radius: 14px; font-weight: bold; color: #A3AED0; font-size: 18px;");
    } else if (typeName == "Deluxe") {
        m_sizeLabel->setText("📏 35m²");
        m_bedLabel->setText("🛏 King Bed");
        m_guestLabel->setText(QString("👤 %1 Guests").arg(m_room->getMaximumGuests()));
        m_extraFeeLabel->setText(QString("💰 Mini Bar Fee: %1 VND").arg(formatMoney(m_room->getExtraFeeAmount())));
        m_extraFeeLabel->setVisible(true);
        m_imageLabel->setText("✨ Deluxe Room Image Placeholder");
        m_imageLabel->setStyleSheet("background-color: #E0F2FE; border-radius: 14px; font-weight: bold; color: #0284C7; font-size: 18px;");
    } else if (typeName == "Suite") {
        m_sizeLabel->setText("📏 55m²");
        m_bedLabel->setText("🛏 Super King Bed");
        m_guestLabel->setText(QString("👤 %1 Guests").arg(m_room->getMaximumGuests()));
        m_extraFeeLabel->setText(QString("👑 Premium Service Fee: %1 VND").arg(formatMoney(m_room->getExtraFeeAmount())));
        m_extraFeeLabel->setVisible(true);
        m_imageLabel->setText("👑 Suite Room Image Placeholder");
        m_imageLabel->setStyleSheet("background-color: #FEF3C7; border-radius: 14px; font-weight: bold; color: #D97706; font-size: 18px;");
    }
}

void RoomInfoDialog::onCancelClicked() {
    reject();
}

void RoomInfoDialog::onBookingClicked() {
    accept();
}
