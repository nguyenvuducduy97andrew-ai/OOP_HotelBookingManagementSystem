#include "RoomDialog.h"
#include "DialogWindowBehavior.h"
#include "SchedulePickerDialog.h"
#include "CustomAlertDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QLocale>
#include <QDate>
#include <QColor>
#include <QListWidget>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionButton>

class AmenityButton : public QPushButton {
public:
    AmenityButton(const QString& text, QWidget* parent = nullptr) : QPushButton(text, parent) {
        setCursor(Qt::PointingHandCursor);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        updateStyle();
    }
    
    void setActive(bool active) {
        if (m_isActive != active) {
            m_isActive = active;
            updateStyle();
        }
    }
    
    bool isActive() const { return m_isActive; }
    
    void setChosen(bool chosen) {
        if (m_isChosen != chosen) {
            m_isChosen = chosen;
            updateStyle();
        }
    }
    
    bool isChosen() const { return m_isChosen; }

private:
    bool m_isActive = false;
    bool m_isChosen = false;
    
public:
    void updateStyle() {
        QString style = "QPushButton { border-radius: 16px; padding: 6px 16px; font-weight: 600; font-size: 13px; text-align: center; ";
        if (m_isChosen) { // Chosen state
            style += "background-color: #ECFDF5; color: #05A660; ";
        } else { // Normal/Gray state
            style += "background-color: #F1F5F9; color: #64748B; ";
        }
        if (m_isActive) {
            style += "border: 2px solid #005BFE; ";
        } else {
            style += "border: 2px solid transparent; ";
        }
        style += "}";
        setStyleSheet(style);
    }
};

RoomDialog::RoomDialog(QWidget *parent)
    : QDialog(parent), m_isEditMode(false) {
    setupUI();
    setWindowTitle("Add New Room");
    onTypeChanged(0); // Standard by default
}

RoomDialog::RoomDialog(const QString& roomNum, double basePrice, RoomType type, double extraFee, bool isAvailable,
                       double area, const QString& bedType, int maxGuests, const QString& description, const QString& amenities,
                       QWidget *parent)
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
    // Apply existing values
    m_areaSpin->setValue(area);
    
    // Set bed type
    int bedIndex = m_bedTypeCombo->findText(bedType);
    if (bedIndex >= 0) {
        m_bedTypeCombo->setCurrentIndex(bedIndex);
    } else {
        m_bedTypeCombo->addItem(bedType);
        m_bedTypeCombo->setCurrentText(bedType);
    }
    
    m_maxGuestsSpin->setValue(maxGuests);
    m_descEdit->setText(description);
    
    m_originalAmenities = amenities;
    
    // Load initial amenities keeping unselected ones gray
    int idx = m_typeCombo->currentIndex();
    QStringList baseAmenities;
    if (idx == 0) {
        baseAmenities << "Free Wi-Fi" << "Air Conditioning" << "Flat-screen TV" << "Work Desk" << "Private Bathroom";
    } else if (idx == 1) {
        baseAmenities << "Stylish Furnishings" << "Air Conditioning" << "Free High-Speed Wi-Fi" << "Mini Bar"<< "Smart TV" << "Modern Bathroom";
    } else if (idx == 2) {
        baseAmenities << "Private Balcony" <<  "Separate Living Room" << "Free High-Speed Wi-Fi" << "Bathtub" << "Smart TV" << "Complimentary Bar" << "luxurious Bathroom" << "Premium Service";
    }
    QStringList currentAmenities = amenities.split(", ", Qt::SkipEmptyParts);
    QStringList allAmenities = baseAmenities;
    for (const QString& am : currentAmenities) {
        if (!allAmenities.contains(am)) {
            allAmenities << am;
        }
    }
    populateAmenities(allAmenities, false);
    for (const QString& am : currentAmenities) {
        if (m_amenityButtons.contains(am)) {
            m_amenityButtons[am]->setChosen(true);
        }
    }
}

void setupDialogStyle(QDialog* dialog) {
    // Modified: Use opaque in-app room dialog chrome instead of the platform-dark title bar.
    dialog->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    dialog->setStyleSheet(R"(
        QDialog {
            background-color: #EFF6FF;
            border: 2px solid #93C5FD;
            border-radius: 18px;
        }
        QCheckBox {
            font-size: 13px;
            color: #2B3674;
            font-weight: 500;
        }
        QLabel {
            font-size: 13px;
            color: #2B3674;
            font-weight: 600;
            background: transparent;
            border: none;
        }
        QLineEdit, QComboBox, QDoubleSpinBox, QSpinBox, QDateEdit, QTextEdit {
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
        QLineEdit:focus, QComboBox:focus, QDoubleSpinBox:focus, QSpinBox:focus, QDateEdit:focus, QTextEdit:focus {
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
        QPushButton#btnAddAmenity {
            background-color: #005BFE;
            color: #FFFFFF;
            border-radius: 8px;
            padding: 6px 12px;
            font-weight: 600;
            font-size: 13px;
        }
        QPushButton#btnAddAmenity:hover {
            background-color: #2B7BFF;
        }
        QLabel#roomDialogTitle {
            font-size: 20px;
            font-weight: 800;
            color: #1B3F83;
        }
        QFrame#roomDialogHeader { background:#FFFFFF; border:none; border-bottom:1px solid #E7EDF7; border-top-left-radius:13px; border-top-right-radius:13px; }
        QPushButton#roomDialogClose { background:transparent; color:#64748B; border:none; border-radius:8px; padding:0px; font-family:"Segoe UI"; font-size:12px; font-weight:800; }
        QPushButton#roomDialogClose:hover { background:#F1F5F9; color:#1B3F83; }
    )");
}

void RoomDialog::setupUI() {
    setupDialogStyle(this);
    // Modified: Keep room editing inside the application work area; scrolling belongs to the content rather than a resizable outer window.
    lockDialogToWorkingArea(this, QSize(760, 650));

    auto* mainLayout = new QVBoxLayout(this);
    // Modified: Leave a light-blue perimeter visible around the Room dialog header and scrollable surface.
    mainLayout->setContentsMargins(3, 3, 3, 3);
    mainLayout->setSpacing(0);

    auto* header = new QFrame(this);
    header->setObjectName("roomDialogHeader");
    header->setFixedHeight(52);
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(24, 0, 12, 0);
    // Modified: Use an unmistakable pencil icon when editing an existing room instead of reusing the room/add icon.
    auto* roomDialogIcon = new QLabel(m_isEditMode ? "✎" : "⌂", header);
    roomDialogIcon->setStyleSheet("color:#005BFE; font-size:18px; background:transparent; border:none;");
    auto* roomDialogTitle = new QLabel(m_isEditMode ? "Edit room" : "Add a new room", header);
    roomDialogTitle->setObjectName("roomDialogTitle");
    auto* roomDialogClose = new QPushButton(header);
    roomDialogClose->setObjectName("roomDialogClose");
    styleDialogCloseButton(roomDialogClose);
    headerLayout->addWidget(roomDialogIcon);
    headerLayout->addSpacing(8);
    headerLayout->addWidget(roomDialogTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(roomDialogClose);
    connect(roomDialogClose, &QPushButton::clicked, this, &QDialog::reject);
    enableDialogHeaderDrag(this, header);
    mainLayout->addWidget(header);
    
    QScrollArea* mainScrollArea = new QScrollArea(this);
    mainScrollArea->setWidgetResizable(true);
    mainScrollArea->setFrameShape(QFrame::NoFrame);
    mainScrollArea->setStyleSheet("QScrollArea { background-color: #FFFFFF; border: none; }");
    
    QWidget* scrollContentWidget = new QWidget(mainScrollArea);
    scrollContentWidget->setObjectName("scrollContent");
    scrollContentWidget->setStyleSheet("QWidget#scrollContent { background-color: #FFFFFF; }");
    mainScrollArea->setWidget(scrollContentWidget);

    auto* formLayout = new QFormLayout(scrollContentWidget);
    formLayout->setContentsMargins(24, 24, 24, 8);
    formLayout->setSpacing(12);


    m_roomNumberEdit = new QLineEdit(this);
    m_roomNumberEdit->setPlaceholderText("Example: 101, 302");
    formLayout->addRow("Room Number:", m_roomNumberEdit);

    // Modified: use a comma-grouped locale so every editable VND amount follows the application currency format.
    const QLocale moneyLocale(QLocale::English, QLocale::UnitedStates);

    m_basePriceSpin = new QDoubleSpinBox(this);
    m_basePriceSpin->setRange(0, 100000000);
    m_basePriceSpin->setSingleStep(50000);
    m_basePriceSpin->setSuffix(" VND");
    m_basePriceSpin->setDecimals(0);
    m_basePriceSpin->setLocale(moneyLocale);
    m_basePriceSpin->setGroupSeparatorShown(true);
    m_basePriceSpin->setProperty("allowMouseWheelValueChange", true);
    // Modified: Present the room rate as an hourly rate because timestamp invoices bill actual elapsed stay time in rounded hours.
    formLayout->addRow("Base Price (per hour):", m_basePriceSpin);

    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItems({"Standard", "Deluxe", "Suite"});
    formLayout->addRow("Room Type:", m_typeCombo);

    m_availabilityCombo = new QComboBox(this);
    m_availabilityCombo->addItems({"Available", "Schedule maintenance"});
    formLayout->addRow("Status:", m_availabilityCombo);

    m_maintenanceStartDate = QDate::currentDate();
    m_maintenanceEndDate = QDate::currentDate();
    m_maintenanceScheduleLabel = new QLabel("Maintenance schedule:", this);
    auto* maintenanceScheduleWidget = new QWidget(this);
    auto* maintenanceScheduleLayout = new QHBoxLayout(maintenanceScheduleWidget);
    maintenanceScheduleLayout->setContentsMargins(0, 0, 0, 0);
    maintenanceScheduleLayout->setSpacing(10);
    m_maintenanceScheduleButton = new QPushButton("Choose dates", maintenanceScheduleWidget);
    m_maintenanceScheduleButton->setObjectName("btnMaintenanceSchedule");
    m_maintenanceScheduleButton->setFixedSize(150, 38);
    // Modified: Give the compact maintenance-date action an explicit opaque style so inherited button colors can never hide its text.
    m_maintenanceScheduleButton->setStyleSheet(
        "QPushButton { background:#EAF2FF; color:#005BFE; border:1px solid #93C5FD; border-radius:8px; padding:7px 12px; font-size:13px; font-weight:700; }"
        "QPushButton:hover { background:#DBEAFE; border-color:#60A5FA; }"
        "QPushButton:pressed { background:#BFDBFE; }");
    m_maintenanceScheduleSummary = new QLabel(maintenanceScheduleWidget);
    m_maintenanceScheduleSummary->setStyleSheet("color:#6B7FA8; font-size:12px;");
    m_maintenanceScheduleSummary->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_maintenanceScheduleSummary->setWordWrap(false);
    m_maintenanceScheduleSummary->setText(QString("%1 → %2 (full days)")
        .arg(m_maintenanceStartDate.toString("dd MMM yyyy"), m_maintenanceEndDate.toString("dd MMM yyyy")));
    maintenanceScheduleLayout->addWidget(m_maintenanceScheduleButton);
    // Modified: Keep the selected full-day range immediately to the right of the compact picker button for quick review.
    maintenanceScheduleLayout->addWidget(m_maintenanceScheduleSummary, 1);
    formLayout->addRow(m_maintenanceScheduleLabel, maintenanceScheduleWidget);

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
    m_extraFeeSpin->setProperty("allowMouseWheelValueChange", true);
    formLayout->addRow(m_extraFeeLabel, m_extraFeeSpin);

    m_areaSpin = new QDoubleSpinBox(this);
    m_areaSpin->setRange(0, 1000);
    m_areaSpin->setSingleStep(5);
    m_areaSpin->setSuffix(" m2");
    formLayout->addRow("Area:", m_areaSpin);

    m_bedTypeCombo = new QComboBox(this);
    m_bedTypeCombo->addItems({"Single Bed", "Twin Beds", "Queen Bed", "King Bed", "Super King Bed"});
    formLayout->addRow("Bed Type:", m_bedTypeCombo);

    m_maxGuestsSpin = new QSpinBox(this);
    m_maxGuestsSpin->setRange(1, 10);
    formLayout->addRow("Max Guests:", m_maxGuestsSpin);

    m_descEdit = new QTextEdit(this);
    m_descEdit->setFixedHeight(60);
    m_descEdit->setAlignment(Qt::AlignJustify);
    formLayout->addRow("Description:", m_descEdit);

    m_amenitiesScrollArea = new QScrollArea(this);
    m_amenitiesScrollArea->setFixedHeight(120);
    m_amenitiesScrollArea->setMinimumWidth(380); // Fix horizontal clipping
    m_amenitiesScrollArea->setWidgetResizable(true);
    m_amenitiesScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_amenitiesScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_amenitiesWidget = new QWidget();
    m_amenitiesWidget->setObjectName("amenitiesWidget");
    
    auto* amenitiesVBox = new QVBoxLayout(m_amenitiesWidget);
    amenitiesVBox->setContentsMargins(0, 0, 0, 0);
    
    m_amenitiesGridLayout = new QGridLayout();
    m_amenitiesGridLayout->setContentsMargins(5, 5, 5, 5);
    m_amenitiesGridLayout->setSpacing(5);
    m_amenitiesGridLayout->setColumnStretch(0, 1);
    m_amenitiesGridLayout->setColumnStretch(1, 1);
    m_amenitiesGridLayout->setColumnStretch(2, 1);
    
    amenitiesVBox->addLayout(m_amenitiesGridLayout);
    amenitiesVBox->addStretch();
    
    m_amenitiesScrollArea->setWidget(m_amenitiesWidget);
    m_amenitiesScrollArea->setStyleSheet("QScrollArea { background-color: #FFFFFF; border: 1px solid #E9EDF7; border-radius: 8px; } QWidget#amenitiesWidget { background-color: transparent; }");
    
    auto* customAmenityLayout = new QHBoxLayout();
    m_customAmenityEdit = new QLineEdit(this);
    m_customAmenityEdit->setPlaceholderText("Add custom amenity...");
    m_addAmenityBtn = new QPushButton("Add", this);
    m_addAmenityBtn->setObjectName("btnAddAmenity");
    customAmenityLayout->addWidget(m_customAmenityEdit);
    customAmenityLayout->addWidget(m_addAmenityBtn);
    
    auto* actionsLayout = new QVBoxLayout();
    actionsLayout->setSpacing(8);
    m_chooseAmenityBtn = new QPushButton("Choose", this);
    m_deleteAmenityBtn = new QPushButton("Delete", this);
    m_resetAmenityBtn = new QPushButton("Reset", this);
    
    m_chooseAmenityBtn->setStyleSheet("QPushButton { background-color: #005BFE; color: white; border-radius: 8px; padding: 6px 12px; font-weight: bold; } QPushButton:hover { background-color: #004ecc; }");
    m_deleteAmenityBtn->setStyleSheet("QPushButton { background-color: #FEE2E2; color: #DC2626; border-radius: 8px; padding: 6px 12px; font-weight: bold; } QPushButton:hover { background-color: #FCA5A5; }");
    m_resetAmenityBtn->setStyleSheet("QPushButton { background-color: #F1F5F9; color: #475569; border-radius: 8px; padding: 6px 12px; font-weight: bold; } QPushButton:hover { background-color: #E2E8F0; }");
    
    // Modified: Amenities are direct toggles; a separate Choose action made selection ambiguous and added an unnecessary step.
    m_chooseAmenityBtn->setVisible(false);
    m_deleteAmenityBtn->setVisible(false);
    
    actionsLayout->addWidget(m_deleteAmenityBtn);
    actionsLayout->addWidget(m_resetAmenityBtn);
    
    auto* amenitiesContainer = new QVBoxLayout();
    amenitiesContainer->setContentsMargins(0,0,0,0);
    amenitiesContainer->addWidget(m_amenitiesScrollArea);
    amenitiesContainer->addLayout(customAmenityLayout);
    
    QWidget* labelWidget = new QWidget();
    auto* labelLayout = new QVBoxLayout(labelWidget);
    labelLayout->setContentsMargins(0, 0, 0, 0);
    labelLayout->addWidget(new QLabel("Amenities:"));
    labelLayout->addLayout(actionsLayout);
    labelLayout->addStretch();
    
    formLayout->addRow(labelWidget, amenitiesContainer);

    mainLayout->addWidget(mainScrollArea);

    // Button Row
    auto* btnLayout = new QHBoxLayout();
    btnLayout->setContentsMargins(24, 8, 24, 24);
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
    connect(m_maintenanceScheduleButton, &QPushButton::clicked, this, &RoomDialog::openMaintenanceSchedulePicker);
    connect(m_cancelMaintenanceBtn, &QPushButton::clicked, this, &RoomDialog::markSelectedMaintenanceForCancellation);
    connect(m_confirmMaintenanceBtn, &QPushButton::clicked, this, &RoomDialog::markSelectedMaintenanceForConfirmation);
    connect(m_addAmenityBtn, &QPushButton::clicked, this, &RoomDialog::onAddCustomAmenity);
    connect(m_existingMaintenanceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        const bool pendingGuestResponse = m_existingMaintenanceCombo->currentData(Qt::UserRole + 1).toString() == "Awaiting guest response";
        m_confirmMaintenanceBtn->setEnabled(pendingGuestResponse);
    });
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(saveBtn, &QPushButton::clicked, this, &RoomDialog::onAccept);
    
    connect(m_deleteAmenityBtn, &QPushButton::clicked, this, [this]() {
        if (m_activeAmenity) {
            QString amText = m_activeAmenity->text();
            m_activeAmenity = nullptr;
            m_chooseAmenityBtn->setVisible(false);
            m_deleteAmenityBtn->setVisible(false);
            onRemoveAmenity(amText);
        }
    });
    
    connect(m_resetAmenityBtn, &QPushButton::clicked, this, &RoomDialog::resetAmenities);
    
    disableNonMoneyWheelChanges(this);
    onStatusChanged(m_availabilityCombo->currentIndex());
}

void RoomDialog::onTypeChanged(int index) {
    if (index == 0) { // Standard
        m_extraFeeLabel->setVisible(false);
        m_extraFeeSpin->setVisible(false);
        if (!m_isEditMode) {
            m_areaSpin->setValue(25);
            m_maxGuestsSpin->setValue(2);
            m_bedTypeCombo->setCurrentText("Queen Bed");
            m_descEdit->setText("Our Standard Room offers a comfortable and affordable stay, perfect for solo travelers or couples. The room features a cozy queen-size bed, air conditioning, free Wi-Fi, a flat-screen TV, a work desk, and a private bathroom with complimentary toiletries. It provides everything you need for a pleasant and relaxing stay.");
        }
    } else if (index == 1) { // Deluxe
        m_extraFeeLabel->setText("Mini Bar Fee:");
        m_extraFeeLabel->setVisible(true);
        m_extraFeeSpin->setVisible(true);
        if (!m_isEditMode) {
            m_areaSpin->setValue(35);
            m_maxGuestsSpin->setValue(3);
            m_bedTypeCombo->setCurrentText("King Bed");
            m_descEdit->setText("The Deluxe Room provides extra space and enhanced comfort for guests seeking a more enjoyable experience. It includes a king-size bed, stylish furnishings, a seating area, large windows with city or garden views, free high-speed Wi-Fi, a smart TV, a minibar, and a modern bathroom equipped with premium amenities. It is an excellent choice for both business and leisure travelers.");
        }
    } else if (index == 2) { // Suite
        m_extraFeeLabel->setText("Premium Service Fee:");
        m_extraFeeLabel->setVisible(true);
        m_extraFeeSpin->setVisible(true);
        if (!m_isEditMode) {
            m_areaSpin->setValue(55);
            m_maxGuestsSpin->setValue(4);
            m_bedTypeCombo->setCurrentText("Super King Bed");
            m_descEdit->setText("Our Suite is designed for guests who appreciate luxury, elegance, and privacy. Featuring a spacious bedroom, a separate living room, and a luxurious bathroom with a bathtub and rain shower, the suite offers a premium experience. Guests can also enjoy a private balcony, complimentary minibar, coffee machine, high-speed Wi-Fi, smart TV, and personalized services, making it ideal for families, special occasions, or extended stays.");
        }
    }

    if (!m_isEditMode) {
        resetAmenities();
    }
    adjustSize();
}

void RoomDialog::onAccept() {
    if (m_roomNumberEdit->text().trimmed().isEmpty()) {
        CustomAlertDialog("Missing information", "Please enter the room number.", this).exec();
        return;
    }
    if (m_basePriceSpin->value() <= 0) {
        // Modified: Use the branded one-action alert so the acknowledgement button remains visible and the validation popup has a soft, consistent frame.
        CustomAlertDialog("Invalid value", "Please enter a room price greater than 0.", this).exec();
        return;
    }
    if (shouldScheduleMaintenance() && m_maintenanceEndDate < m_maintenanceStartDate) {
        // Modified: Permit a one-day Maintenance case while preserving the selected end day as fully unavailable.
        CustomAlertDialog("Invalid maintenance dates", "Maintenance cannot end before its start date.", this).exec();
        return;
    }
    if ((!m_maintenanceIdToCancel.isEmpty() || !m_maintenanceIdToConfirm.isEmpty()) && shouldScheduleMaintenance()) {
        CustomAlertDialog("Complete one maintenance action",
                          "Save the schedule cancellation first, then create a new maintenance schedule.",
                          this).exec();
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

double RoomDialog::getExtraFee() const { return m_extraFeeSpin->value(); }
bool RoomDialog::getIsAvailable() const { return m_availabilityCombo->currentIndex() == 0; }
double RoomDialog::getArea() const { return m_areaSpin->value(); }
QString RoomDialog::getBedType() const { return m_bedTypeCombo->currentText(); }
int RoomDialog::getMaxGuests() const { return m_maxGuestsSpin->value(); }
QString RoomDialog::getDescription() const { return m_descEdit->toPlainText(); }
QString RoomDialog::getAmenities() const {
    QStringList checkedAmenities;
    for (auto it = m_amenityButtons.begin(); it != m_amenityButtons.end(); ++it) {
        if (it.value()->isChosen()) {
            checkedAmenities << it.key();
        }
    }
    return checkedAmenities.join(", ");
}
bool RoomDialog::shouldScheduleMaintenance() const { return m_availabilityCombo->currentIndex() == 1; }

QString RoomDialog::getMaintenanceStartDate() const {
    return m_maintenanceStartDate.toString(Qt::ISODate);
}

QString RoomDialog::getMaintenanceEndDate() const {
    return m_maintenanceEndDate.toString(Qt::ISODate);
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
        const QDate exclusiveEnd = QDate::fromString(QString::fromStdString(maintenance.getEndDate()), Qt::ISODate);
        const QString inclusiveEnd = exclusiveEnd.isValid()
            ? exclusiveEnd.addDays(-1).toString(Qt::ISODate)
            : QString::fromStdString(maintenance.getEndDate());
        QString label = QString("%1 | %2 to %3")
            .arg(QString::fromStdString(maintenance.getStatus()))
            .arg(QString::fromStdString(maintenance.getStartDate()))
            .arg(inclusiveEnd);
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

    m_maintenanceIdToCancel = m_existingMaintenanceCombo->currentData().toString();
    m_existingMaintenanceLabel->setText("Cancelled");
    m_existingMaintenanceCombo->setEnabled(false);
    m_cancelMaintenanceBtn->setText("Cancellation selected");
    m_cancelMaintenanceBtn->setEnabled(false);
}

void RoomDialog::markSelectedMaintenanceForConfirmation() {

    // Modified: Close immediately after selecting confirmation; the parent workflow rechecks live booking conflicts before persistence.
    m_maintenanceIdToConfirm = m_existingMaintenanceCombo->currentData().toString();
    accept();
}

void RoomDialog::onStatusChanged(int index) {
    const bool scheduleMaintenance = index == 1;
    m_maintenanceScheduleLabel->setVisible(scheduleMaintenance);
    m_maintenanceScheduleButton->parentWidget()->setVisible(scheduleMaintenance);
    m_maintenanceNoteLabel->setVisible(scheduleMaintenance);
    m_maintenanceNoteEdit->setVisible(scheduleMaintenance);
    adjustSize();
}

void RoomDialog::openMaintenanceSchedulePicker()
{
    // Modified: Maintenance create/edit now uses the same branded range calendar as room booking while retaining inclusive full-day semantics.
    SchedulePickerDialog picker(QDateTime(m_maintenanceStartDate, QTime(0, 0)),
                                QDateTime(m_maintenanceEndDate, QTime(0, 0)),
                                {}, this, false, false,
                                SchedulePickerDialog::Purpose::MaintenanceRange);
    if (picker.exec() != QDialog::Accepted) {
        return;
    }
    m_maintenanceStartDate = picker.selectedCheckIn().date();
    m_maintenanceEndDate = picker.selectedCheckOut().date();
    m_maintenanceScheduleSummary->setText(QString("%1 → %2 (full days)")
        .arg(m_maintenanceStartDate.toString("dd MMM yyyy"), m_maintenanceEndDate.toString("dd MMM yyyy")));
}

void RoomDialog::onAddCustomAmenity() {
    QString text = m_customAmenityEdit->text().trimmed();
    if (!text.isEmpty()) {
        if (!m_amenityButtons.contains(text)) {
            QStringList current;
            for (auto it = m_amenityButtons.begin(); it != m_amenityButtons.end(); ++it) {
                current << it.key();
            }
            current << text;
            populateAmenities(current, true);
        } else {
            m_amenityButtons[text]->setChosen(true);
        }
        m_customAmenityEdit->clear();
    }
}

void RoomDialog::populateAmenities(const QStringList& amenities, bool checkAll) {
    // Determine which ones are currently chosen so we can preserve state if they still exist
    QStringList currentlyChosen;
    for (auto it = m_amenityButtons.begin(); it != m_amenityButtons.end(); ++it) {
        if (it.value()->isChosen()) {
            currentlyChosen << it.key();
        }
    }

    // Clear layout
    QLayoutItem* item;
    while ((item = m_amenitiesGridLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    m_amenityButtons.clear();
    m_activeAmenity = nullptr;
    m_deleteAmenityBtn->setVisible(false);

    // Rebuild grid (now with flow-like wrapping but we will just use grid 3 columns)
    int row = 0;
    int col = 0;
    for (int i = 0; i < amenities.size(); ++i) {
        const QString& am = amenities[i];
        
        AmenityButton* btn = new AmenityButton(am, this);
        if (checkAll || currentlyChosen.contains(am)) {
            btn->setChosen(true);
        }
        m_amenityButtons.insert(am, btn);

        connect(btn, &QPushButton::clicked, this, [this, btn]() {
            // Modified: Clicking an amenity now selects it and clicking again clears it, matching normal multi-select controls.
            btn->setChosen(!btn->isChosen());
            if (m_activeAmenity && m_activeAmenity != btn) {
                m_activeAmenity->setActive(false);
            }
            m_activeAmenity = btn;
            btn->setActive(true);
            // Deleting an amenity is only an editing aid; it is no longer part of the choose/unchoose interaction.
            m_deleteAmenityBtn->setVisible(true);
        });

        m_amenitiesGridLayout->addWidget(btn, row, col);
        
        col++;
        if (col >= 3) {
            col = 0;
            row++;
        }
    }
}

void RoomDialog::onRemoveAmenity(const QString& amenityText) {
    QStringList current;
    for (auto it = m_amenityButtons.begin(); it != m_amenityButtons.end(); ++it) {
        if (it.key() != amenityText) {
            current << it.key();
        }
    }
    populateAmenities(current, false);
}

void RoomDialog::mousePressEvent(QMouseEvent *event) {
    if (m_activeAmenity) {
        m_activeAmenity->setActive(false);
        m_activeAmenity = nullptr;
        m_chooseAmenityBtn->setVisible(false);
        m_deleteAmenityBtn->setVisible(false);
    }
    QDialog::mousePressEvent(event);
}

void RoomDialog::resetAmenities() {
    int index = m_typeCombo->currentIndex();
    QStringList baseAmenities;
    if (index == 0) {
        baseAmenities << "Free Wi-Fi" << "Air Conditioning" << "Flat-screen TV" << "Work Desk" << "Private Bathroom";
    } else if (index == 1) {
        baseAmenities << "Stylish Furnishings" << "Air Conditioning" << "Free High-Speed Wi-Fi" << "Mini Bar"<< "Smart TV" << "Modern Bathroom";
    } else if (index == 2) {
        baseAmenities << "Private Balcony" <<  "Separate Living Room" << "Free High-Speed Wi-Fi" << "Bathtub" << "Smart TV" << "Complimentary Bar" << "luxurious Bathroom" << "Premium Service";
    }

    if (m_isEditMode) {
        QStringList currentAmenities = m_originalAmenities.split(", ", Qt::SkipEmptyParts);
        QStringList allAmenities = baseAmenities;
        for (const QString& am : currentAmenities) {
            if (!allAmenities.contains(am)) {
                allAmenities << am;
            }
        }
        populateAmenities(allAmenities, true);
    } else {
        populateAmenities(baseAmenities, true);
    }
}
