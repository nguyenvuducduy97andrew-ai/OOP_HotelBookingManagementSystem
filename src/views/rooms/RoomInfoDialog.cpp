#include "RoomInfoDialog.h"
#include "DialogWindowBehavior.h"
#include "RoomImageCarousel.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QTextEdit>
#include <QLocale>

RoomInfoDialog::RoomInfoDialog(std::shared_ptr<Room> room, QWidget *parent)
    : QDialog(parent), m_room(std::move(room)) {
    
    setupUI();
    setupStyle();
    populateData();
    
    // Modified: Compact Room Info through whitespace and media sizing, while retaining the established typography and visual hierarchy.
    lockDialogToWorkingArea(this, QSize(540, 640));
    setWindowTitle("Room Information");
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    // Modified: Keep the room-information step opaque so it remains visually consistent with functional dialogs.
    setAttribute(Qt::WA_TranslucentBackground, false);
}

QString RoomInfoDialog::formatMoney(double amount) {
    // Modified: Format room information prices locally with comma thousands separators.
    return QLocale(QLocale::English, QLocale::UnitedStates).toString(amount, 'f', 0);
}

void RoomInfoDialog::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    
    auto* bgFrame = new QFrame(this);
    bgFrame->setObjectName("bgFrame");
    auto* frameLayout = new QVBoxLayout(bgFrame);
    frameLayout->setContentsMargins(16, 16, 16, 16);
    frameLayout->setSpacing(7);
    
    // Header row
    auto* headerLayout = new QHBoxLayout();
    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName("titleLabel");
    
    m_statusBadge = new QLabel("Available", this);
    m_statusBadge->setObjectName("statusBadge");
    
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addWidget(m_statusBadge);
    headerLayout->addStretch();
    enableDialogHeaderDrag(this, m_titleLabel);
    
    // Image area
    m_imageLabel = new QLabel(this);
    m_imageLabel->setObjectName("imageLabel");
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setFixedHeight(130);
    // Modified: Keep Suite media within the same vertical budget as the placeholder so the fixed Room Info layout remains orderly.
    m_roomImageCarousel = new RoomImageCarousel(105, this);
    m_roomImageCarousel->hide();
    
    // Specs row
    auto* specsLayout = new QHBoxLayout();
    m_sizeLabel = new QLabel(this);
    m_sizeLabel->setObjectName("specLabel");
    m_bedLabel = new QLabel(this);
    m_bedLabel->setObjectName("specLabel");
    m_guestLabel = new QLabel(this);
    m_guestLabel->setObjectName("specLabel");
    
    specsLayout->setContentsMargins(2, 0, 2, 0);
    specsLayout->setSpacing(8);
    specsLayout->addWidget(m_sizeLabel, 1, Qt::AlignLeft | Qt::AlignVCenter);
    specsLayout->addWidget(m_bedLabel, 2, Qt::AlignCenter);
    specsLayout->addWidget(m_guestLabel, 1, Qt::AlignRight | Qt::AlignVCenter);
    
    // Hourly base price and optional room-type fee
    m_basePriceLabel = new QLabel(this);
    m_basePriceLabel->setObjectName("basePriceLabel");
    m_basePriceLabel->setAlignment(Qt::AlignCenter);

    m_extraFeeLabel = new QLabel(this);
    m_extraFeeLabel->setObjectName("extraFeeLabel");
    m_extraFeeLabel->setAlignment(Qt::AlignCenter);
    
    // Description
    auto* descTitle = new QLabel("Room Description", this);
    descTitle->setObjectName("sectionTitle");
    m_descLabel = new QTextEdit(this);
    m_descLabel->setObjectName("descLabel");
    m_descLabel->setReadOnly(true);
    m_descLabel->setFrameShape(QFrame::NoFrame);
    m_descLabel->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_descLabel->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_descLabel->setFixedHeight(54);
    m_descLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_descLabel->setAlignment(Qt::AlignJustify);
    
    // Amenities
    auto* amTitle = new QLabel("Amenities", this);
    amTitle->setObjectName("sectionTitle");
    m_amContainer = new QWidget(this);
    m_amLayout = new QGridLayout(m_amContainer);
    m_amLayout->setContentsMargins(0, 0, 0, 0);
    m_amLayout->setHorizontalSpacing(8);
    m_amLayout->setVerticalSpacing(4);
    m_amLayout->setColumnStretch(0, 1);
    m_amLayout->setColumnStretch(1, 1);
    
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
    frameLayout->addWidget(m_roomImageCarousel);
    frameLayout->addLayout(specsLayout);
    frameLayout->addWidget(m_basePriceLabel);
    frameLayout->addWidget(m_extraFeeLabel);
    frameLayout->addWidget(descTitle);
    frameLayout->addWidget(m_descLabel);
    frameLayout->addWidget(amTitle);
    frameLayout->addWidget(m_amContainer);
    frameLayout->addStretch();
    frameLayout->addLayout(btnLayout);
    
    mainLayout->addWidget(bgFrame);
    
    connect(m_btnCancel, &QPushButton::clicked, this, &RoomInfoDialog::onCancelClicked);
    connect(m_btnBooking, &QPushButton::clicked, this, &RoomInfoDialog::onBookingClicked);
}

void RoomInfoDialog::setupStyle() {
    setStyleSheet(R"(
        QDialog {
            background-color: #EFF6FF;
            border: 2px solid #93C5FD;
            border-radius: 18px;
        }
        QFrame#bgFrame {
            background-color: #FFFFFF;
            border-radius: 20px;
            border: 1px solid #D9EAFE;
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
            color: #3B58FF;
            padding: 8px;
            border-radius: 8px;
            font-weight: 700;
            font-size: 14px;
        }
        QLabel#basePriceLabel {
            background-color: #EFF6FF;
            color: #1D4ED8;
            border: 1px solid #BFDBFE;
            padding: 8px;
            border-radius: 8px;
            font-weight: 800;
            font-size: 15px;
        }
        QLabel#sectionTitle {
            font-size: 16px;
            font-weight: 700;
            color: #1B2559;
        }
        QTextEdit#descLabel {
            font-size: 14px;
            color: #A3AED0;
            background-color: transparent;
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
            padding: 10px 20px;
            font-size: 14px;
            border: none;
        }
        QPushButton#btnCancel:hover {
            background-color: #E9EDF7;
        }
        QPushButton#btnBooking {
            background-color: #3B58FF;
            color: #FFFFFF;
            font-weight: 700;
            border-radius: 12px;
            padding: 10px 20px;
            font-size: 14px;
            border: none;
        }
        QPushButton#btnBooking:hover {
            background-color: #4F6BFF;
        }
    )");
}

void RoomInfoDialog::populateData() {
    if (!m_room) return;
    
    m_titleLabel->setText(QString("Room %1").arg(QString::fromStdString(m_room->getRoomNumber())));
    
    QString desc = QString::fromStdString(m_room->getDescription());
    if (desc.isEmpty()) {
        desc = "This room is currently available and can be booked for your selected dates.";
    }
    m_descLabel->setHtml("<div align=\"justify\">" + desc + "</div>");
    
    QString rawAmenities = QString::fromStdString(m_room->getAmenities());
    QStringList amList = rawAmenities.split(",", Qt::SkipEmptyParts);
    
    QLayoutItem *child;
    while ((child = m_amLayout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }
    
    int row = 0;
    int col = 0;
    bool hasAmenities = false;
    for (QString am : amList) {
        am = am.trimmed();
        if (am.isEmpty()) continue;
        hasAmenities = true;
        
        QLabel* pill = new QLabel("✓ " + am, m_amContainer);
        pill->setStyleSheet("background-color: #F4F7FE; color: #3B58FF; border: 1px solid #E9EDF7; padding: 6px 12px; border-radius: 12px; font-weight: 600; font-size: 13px;");
        m_amLayout->addWidget(pill, row, col);
        
        col++;
        if (col >= 2) {
            col = 0;
            row++;
        }
    }
    
    if (!hasAmenities) {
        QLabel* emptyLabel = new QLabel("No amenities listed.", m_amContainer);
        emptyLabel->setStyleSheet("color: #A3AED0; font-size: 14px; font-style: italic;");
        m_amLayout->addWidget(emptyLabel, 0, 0);
    } else {
        m_amLayout->setColumnStretch(0, 1);
        m_amLayout->setColumnStretch(1, 1);
    }
    
    const std::string typeName = m_room->getRoomTypeName();
    
    double area = m_room->getArea();
    if (area <= 0) {
        if (typeName == "Standard") area = 25;
        else if (typeName == "Deluxe") area = 35;
        else area = 55;
    }
    m_sizeLabel->setText(QString("📏 %1m²").arg(area));
    
    QString bedType = QString::fromStdString(m_room->getBedType());
    if (bedType.isEmpty()) {
        if (typeName == "Standard") bedType = "Queen Bed";
        else if (typeName == "Deluxe") bedType = "King Bed";
        else bedType = "Super King Bed";
    }
    m_bedLabel->setText(QString("🛏 %1").arg(bedType));
    
    m_guestLabel->setText(QString("👤 %1 Guests").arg(m_room->getMaximumGuests()));
    m_basePriceLabel->setText(QString("Base price: %1 VND / hour")
        .arg(formatMoney(m_room->getBasePrice())));

    if (typeName == "Standard") {
        // Modified: Use the same navigable, softly previewed gallery for Standard rooms as for Suite rooms.
        m_imageLabel->hide();
        m_roomImageCarousel->setGallery("Standard", 6);
        m_roomImageCarousel->show();
        m_roomImageCarousel->updateGeometry();
        if (layout()) {
            layout()->activate();
        }
        m_extraFeeLabel->setVisible(false);
    } else if (typeName == "Deluxe") {
        // Modified: Replace the Deluxe placeholder with the shared room gallery and its adjacent-image previews.
        m_imageLabel->hide();
        m_roomImageCarousel->setGallery("Deluxe", 8);
        m_roomImageCarousel->show();
        m_roomImageCarousel->updateGeometry();
        if (layout()) {
            layout()->activate();
        }
        m_extraFeeLabel->setText(QString("💰 Mini Bar Fee: %1 VND").arg(formatMoney(m_room->getExtraFeeAmount())));
        m_extraFeeLabel->setVisible(true);
    } else if (typeName == "Suite") {
        // Modified: Replace the Suite placeholder with a navigable center-focused gallery while keeping adjacent images softly visible.
        m_imageLabel->hide();
        m_roomImageCarousel->setGallery("Suite", 9);
        m_roomImageCarousel->show();
        // Modified: Recalculate the fixed dialog layout after replacing its hidden placeholder with the Suite gallery.
        m_roomImageCarousel->updateGeometry();
        if (layout()) {
            layout()->activate();
        }
        m_extraFeeLabel->setText(QString("👑 Premium Service Fee: %1 VND").arg(formatMoney(m_room->getExtraFeeAmount())));
        m_extraFeeLabel->setVisible(true);
    }
}

void RoomInfoDialog::onCancelClicked() {
    if (m_embedded) {
        emit cancelClicked();
        return;
    }
    reject();
}

void RoomInfoDialog::onBookingClicked() {
    if (m_embedded) {
        emit bookingClicked();
        return;
    }
    accept();
}

void RoomInfoDialog::setEmbeddedMode(bool embedded) {
    m_embedded = embedded;
    if (!embedded) return;

    setWindowFlags(Qt::Widget);
    setMinimumSize(480, 0);
    setMaximumSize(480, QWIDGETSIZE_MAX);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    show();
}
