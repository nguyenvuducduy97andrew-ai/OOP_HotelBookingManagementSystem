#include "ReservationsPageWidget.h"
#include "ReservationDialog.h"
#include "Customer.h"
#include "Room.h"
#include "Invoice.h"
#include "CustomSuccessDialog.h"
#include "CustomConfirmDialog.h"
#include "InvoiceDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>
#include <QDate>
#include <QDebug>

ReservationsPageWidget::ReservationsPageWidget(HotelManager* manager, QWidget *parent)
    : QWidget(parent), m_manager(manager), m_statusFilterIndex(0) {
    setupUI();
    refreshData();
}

void setupReservationsPageStyle(QWidget* widget) {
    widget->setStyleSheet(R"(
        QLabel#pageTitle {
            font-size: 20px;
            font-weight: 800;
            color: #2B3674;
        }
        QLineEdit#searchEdit {
            background-color: #F4F7FE;
            border: 1px solid #E9EDF7;
            border-radius: 10px;
            padding: 8px 16px;
            font-size: 13px;
            color: #2B3674;
            min-width: 250px;
        }
        QLineEdit#searchEdit:focus {
            border: 1px solid #005BFE;
        }
        QComboBox#statusCombo {
            background-color: #F4F7FE;
            border: 1px solid #E9EDF7;
            border-radius: 10px;
            padding: 8px 16px;
            font-size: 13px;
            color: #2B3674;
            min-width: 150px;
        }
        QComboBox QAbstractItemView {
            background-color: #FFFFFF;
            color: #2B3674;
            selection-background-color: #005BFE;
            selection-color: #FFFFFF;
            border: 1px solid #E9EDF7;
        }
        QPushButton#btnAddBooking {
            background-color: #005BFE;
            color: #FFFFFF;
            font-weight: 700;
            border-radius: 10px;
            padding: 8px 18px;
            font-size: 13px;
            border: none;
        }
        QPushButton#btnAddBooking:hover {
            background-color: #2B7BFF;
        }
        QTableWidget {
            border: 1px solid #E9EDF7;
            border-radius: 14px;
            gridline-color: #F1F5F9;
            background-color: #FFFFFF;
            font-size: 13px;
            color: #2B3674;
        }
        QTableWidget::item {
            padding: 4px 10px;
        }
        QHeaderView::section {
            background-color: #F8FAFC;
            color: #A3AED0;
            font-weight: 700;
            border: none;
            border-bottom: 2px solid #E9EDF7;
            padding: 8px;
            font-size: 12px;
        }
    )");
}

void ReservationsPageWidget::setupUI() {
    setupReservationsPageStyle(this);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(16);

    // Top Header Row
    auto* headerRow = new QHBoxLayout();
    auto* pageTitle = new QLabel("Quản lý Đặt phòng (Reservations)", this);
    pageTitle->setObjectName("pageTitle");

    m_addBookingBtn = new QPushButton("Đặt phòng mới", this);
    m_addBookingBtn->setObjectName("btnAddBooking");

    headerRow->addWidget(pageTitle);
    headerRow->addStretch();
    headerRow->addWidget(m_addBookingBtn);
    mainLayout->addLayout(headerRow);

    // Legend Row (Chú thích các biểu tượng thao tác)
    auto* legendRow = new QHBoxLayout();
    legendRow->setSpacing(12);
    legendRow->setAlignment(Qt::AlignLeft);

    auto* legendTitle = new QLabel("Chú thích thao tác:", this);
    legendTitle->setStyleSheet("font-weight: bold; color: #2B3674; font-size: 11px;");
    legendRow->addWidget(legendTitle);

    auto* legCheckOut = new QLabel("💳 Trả phòng", this);
    legCheckOut->setStyleSheet("background-color: #ECFDF5; color: #065F46; border: 1px solid #A7F3D0; font-weight: bold; border-radius: 10px; padding: 2px 8px; font-size: 11px;");
    legendRow->addWidget(legCheckOut);

    auto* legEdit = new QLabel("🖊 Chỉnh sửa", this);
    legEdit->setStyleSheet("background-color: #E9EFFF; color: #1E40AF; border: 1px solid #C3D4FF; font-weight: bold; border-radius: 10px; padding: 2px 8px; font-size: 11px;");
    legendRow->addWidget(legEdit);

    auto* legCancel = new QLabel("❌ Hủy đặt", this);
    legCancel->setStyleSheet("background-color: #FFFBEB; color: #92400E; border: 1px solid #FDE68A; font-weight: bold; border-radius: 10px; padding: 2px 8px; font-size: 11px;");
    legendRow->addWidget(legCancel);

    auto* legDelete = new QLabel("🗑 Xóa / Xóa lịch sử", this);
    legDelete->setStyleSheet("background-color: #FEF2F2; color: #991B1B; border: 1px solid #FCA5A5; font-weight: bold; border-radius: 10px; padding: 2px 8px; font-size: 11px;");
    legendRow->addWidget(legDelete);

    mainLayout->addLayout(legendRow);

    // Filter Row
    auto* filterRow = new QHBoxLayout();
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setObjectName("searchEdit");
    m_searchEdit->setPlaceholderText("Tìm kiếm theo tên khách, số phòng...");

    m_statusCombo = new QComboBox(this);
    m_statusCombo->setObjectName("statusCombo");
    m_statusCombo->addItems({"Tất cả trạng thái", "Sắp tới (Upcoming)", "Đang ở (Active)", "Hoàn tất (Completed)", "Đã hủy (Cancelled)"});

    filterRow->addWidget(m_searchEdit);
    filterRow->addWidget(m_statusCombo);
    filterRow->addStretch();
    mainLayout->addLayout(filterRow);

    // Table Widget
    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setColumnCount(9);
    m_tableWidget->setHorizontalHeaderLabels({
        "Mã Đặt Phòng", "Mã CCCD", "Tên Khách Hàng", "Số Điện Thoại",
        "Số Phòng", "Ngày Nhận", "Ngày Trả", "Trạng Thái", "Thao Tác"
    });
    
    for (int i = 0; i < 8; ++i) {
        m_tableWidget->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    }
    m_tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch); // Let Customer Name take remaining space
    m_tableWidget->horizontalHeader()->setSectionResizeMode(8, QHeaderView::Fixed);
    m_tableWidget->setColumnWidth(8, 140); // 140px is perfect for compact square icon buttons
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableWidget->verticalHeader()->setDefaultSectionSize(42);
    m_tableWidget->verticalHeader()->setVisible(false);

    mainLayout->addWidget(m_tableWidget);

    // Connects
    connect(m_searchEdit, &QLineEdit::textChanged, this, &ReservationsPageWidget::onSearchChanged);
    connect(m_statusCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ReservationsPageWidget::onFilterStatusChanged);
    connect(m_addBookingBtn, &QPushButton::clicked, this, &ReservationsPageWidget::onAddBookingClicked);
}

void ReservationsPageWidget::onSearchChanged(const QString& text) {
    m_searchQuery = text.trimmed();
    refreshData();
}

void ReservationsPageWidget::onFilterStatusChanged(int index) {
    m_statusFilterIndex = index;
    refreshData();
}

void ReservationsPageWidget::onAddBookingClicked() {
    ReservationDialog dialog(m_manager, this);
    if (dialog.exec() == QDialog::Accepted) {
        std::string custId = dialog.getCustomerId().toStdString();
        std::string name = dialog.getCustomerName().toStdString();
        std::string phone = dialog.getCustomerPhone().toStdString();
        std::string roomNum = dialog.getRoomNumber().toStdString();
        std::string checkIn = dialog.getCheckInDate().toStdString();
        std::string checkOut = dialog.getCheckOutDate().toStdString();

        std::string errMsg;
        // 1. Register customer if they do not exist
        if (!m_manager->customerIdExists(custId)) {
            if (!m_manager->registerCustomer(custId, name, phone, errMsg)) {
                QMessageBox::critical(this, "Lỗi đăng ký khách hàng", QString::fromStdString(errMsg));
                return;
            }
        }

        // 2. Create the booking
        if (m_manager->createBooking(custId, roomNum, checkIn, checkOut, errMsg)) {
            refreshData();
            CustomSuccessDialog("Đã thực hiện đặt phòng thành công.", this).exec();
        } else {
            QMessageBox::critical(this, "Lỗi đặt phòng", QString::fromStdString(errMsg));
        }
    }
}

void ReservationsPageWidget::refreshData() {
    m_tableWidget->setRowCount(0);
    if (!m_manager) return;

    int row = 0;
    for (const auto& booking : m_manager->getBookings()) {
        if (!booking) continue;

        // Apply Status Filter
        BookingState state = m_manager->getBookingState(*booking);
        if (m_statusFilterIndex > 0) {
            bool matches = false;
            if (m_statusFilterIndex == 1 && state == BookingState::UPCOMING) matches = true;
            else if (m_statusFilterIndex == 2 && state == BookingState::ACTIVE) matches = true;
            else if (m_statusFilterIndex == 3 && state == BookingState::COMPLETED) matches = true;
            else if (m_statusFilterIndex == 4 && state == BookingState::CANCELLED) matches = true;

            if (!matches) continue;
        }

        // Get associations safely
        auto customer = booking->getCustomer();
        auto room = booking->getRoom();
        if (!customer || !room) continue;

        // Apply Search Filter
        QString custName = QString::fromStdString(customer->getName());
        QString roomNum = QString::fromStdString(room->getRoomNumber());
        QString bId = QString::fromStdString(booking->getBookingId());

        if (!m_searchQuery.isEmpty()) {
            if (!custName.contains(m_searchQuery, Qt::CaseInsensitive) &&
                !roomNum.contains(m_searchQuery, Qt::CaseInsensitive) &&
                !bId.contains(m_searchQuery, Qt::CaseInsensitive)) {
                continue;
            }
        }

        m_tableWidget->insertRow(row);

        // Populate items
        m_tableWidget->setItem(row, 0, new QTableWidgetItem(bId));
        m_tableWidget->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(customer->getCustomerId())));
        m_tableWidget->setItem(row, 2, new QTableWidgetItem(custName));
        m_tableWidget->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(customer->getPhoneNumber())));
        m_tableWidget->setItem(row, 4, new QTableWidgetItem(roomNum));
        m_tableWidget->setItem(row, 5, new QTableWidgetItem(QString::fromStdString(booking->getCheckInDate())));
        m_tableWidget->setItem(row, 6, new QTableWidgetItem(QString::fromStdString(booking->getCheckOutDate())));

        // Status Item & color
        auto* statusItem = new QTableWidgetItem();
        if (state == BookingState::UPCOMING) {
            statusItem->setText("📅 Sắp tới");
            statusItem->setForeground(QColor("#005BFE"));
        } else if (state == BookingState::ACTIVE) {
            statusItem->setText("🛏 Đang ở");
            statusItem->setForeground(QColor("#D97706"));
        } else if (state == BookingState::COMPLETED) {
            statusItem->setText("✔ Hoàn tất");
            statusItem->setForeground(QColor("#05CD99"));
        } else if (state == BookingState::CANCELLED) {
            statusItem->setText("✖ Đã hủy");
            statusItem->setForeground(QColor("#EF4444"));
        }
        m_tableWidget->setItem(row, 7, statusItem);

        // Action Column - Multiple buttons layout container
        auto* actionContainer = new QWidget(m_tableWidget);
        auto* actionLayout = new QHBoxLayout(actionContainer);
        actionLayout->setContentsMargins(0, 0, 0, 0);
        actionLayout->setSpacing(4);
        actionLayout->setAlignment(Qt::AlignCenter);

        if (state == BookingState::ACTIVE) {
            // Check Out
            auto* checkOutBtn = new QPushButton("💳", actionContainer);
            checkOutBtn->setToolTip("Trả phòng & Thanh toán");
            checkOutBtn->setStyleSheet("background-color: #ECFDF5; border: 1px solid #A7F3D0; border-radius: 6px; font-size: 14px; min-width: 28px; max-width: 28px; min-height: 28px; max-height: 28px; padding: 0px;");
            checkOutBtn->setProperty("bookingId", bId);
            checkOutBtn->setProperty("actionType", "checkout");
            connect(checkOutBtn, &QPushButton::clicked, this, &ReservationsPageWidget::onTableActionClicked);
            actionLayout->addWidget(checkOutBtn);

            // Edit
            auto* editBtn = new QPushButton("🖊", actionContainer);
            editBtn->setToolTip("Chỉnh sửa thông tin");
            editBtn->setStyleSheet("background-color: #E9EFFF; border: 1px solid #C3D4FF; border-radius: 6px; font-size: 14px; min-width: 28px; max-width: 28px; min-height: 28px; max-height: 28px; padding: 0px;");
            editBtn->setProperty("bookingId", bId);
            editBtn->setProperty("actionType", "edit");
            connect(editBtn, &QPushButton::clicked, this, &ReservationsPageWidget::onTableActionClicked);
            actionLayout->addWidget(editBtn);

            // Delete
            auto* deleteBtn = new QPushButton("🗑", actionContainer);
            deleteBtn->setToolTip("Xóa đơn đặt phòng");
            deleteBtn->setStyleSheet("background-color: #FEF2F2; border: 1px solid #FCA5A5; border-radius: 6px; font-size: 14px; min-width: 28px; max-width: 28px; min-height: 28px; max-height: 28px; padding: 0px;");
            deleteBtn->setProperty("bookingId", bId);
            deleteBtn->setProperty("actionType", "delete");
            connect(deleteBtn, &QPushButton::clicked, this, &ReservationsPageWidget::onTableActionClicked);
            actionLayout->addWidget(deleteBtn);

        } else if (state == BookingState::UPCOMING) {
            // Cancel
            auto* cancelBtn = new QPushButton("❌", actionContainer);
            cancelBtn->setToolTip("Hủy đơn đặt phòng");
            cancelBtn->setStyleSheet("background-color: #FFFBEB; border: 1px solid #FDE68A; border-radius: 6px; font-size: 13px; min-width: 28px; max-width: 28px; min-height: 28px; max-height: 28px; padding: 0px;");
            cancelBtn->setProperty("bookingId", bId);
            cancelBtn->setProperty("actionType", "cancel");
            connect(cancelBtn, &QPushButton::clicked, this, &ReservationsPageWidget::onTableActionClicked);
            actionLayout->addWidget(cancelBtn);

            // Edit
            auto* editBtn = new QPushButton("🖊", actionContainer);
            editBtn->setToolTip("Chỉnh sửa thông tin");
            editBtn->setStyleSheet("background-color: #E9EFFF; border: 1px solid #C3D4FF; border-radius: 6px; font-size: 14px; min-width: 28px; max-width: 28px; min-height: 28px; max-height: 28px; padding: 0px;");
            editBtn->setProperty("bookingId", bId);
            editBtn->setProperty("actionType", "edit");
            connect(editBtn, &QPushButton::clicked, this, &ReservationsPageWidget::onTableActionClicked);
            actionLayout->addWidget(editBtn);

            // Delete
            auto* deleteBtn = new QPushButton("🗑", actionContainer);
            deleteBtn->setToolTip("Xóa đơn đặt phòng");
            deleteBtn->setStyleSheet("background-color: #FEF2F2; border: 1px solid #FCA5A5; border-radius: 6px; font-size: 14px; min-width: 28px; max-width: 28px; min-height: 28px; max-height: 28px; padding: 0px;");
            deleteBtn->setProperty("bookingId", bId);
            deleteBtn->setProperty("actionType", "delete");
            connect(deleteBtn, &QPushButton::clicked, this, &ReservationsPageWidget::onTableActionClicked);
            actionLayout->addWidget(deleteBtn);
        } else {
            // Completed / Cancelled -> only allow Delete
            auto* deleteBtn = new QPushButton("🗑", actionContainer);
            deleteBtn->setToolTip("Xóa lịch sử đặt phòng");
            deleteBtn->setStyleSheet("background-color: #FEF2F2; border: 1px solid #FCA5A5; border-radius: 6px; font-size: 14px; min-width: 28px; max-width: 28px; min-height: 28px; max-height: 28px; padding: 0px;");
            deleteBtn->setProperty("bookingId", bId);
            deleteBtn->setProperty("actionType", "delete");
            connect(deleteBtn, &QPushButton::clicked, this, &ReservationsPageWidget::onTableActionClicked);
            actionLayout->addWidget(deleteBtn);
        }

        m_tableWidget->setCellWidget(row, 8, actionContainer);

        row++;
    }
}

void ReservationsPageWidget::onTableActionClicked() {
    auto* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    std::string bookingId = btn->property("bookingId").toString().toStdString();
    QString actionType = btn->property("actionType").toString();

    auto booking = m_manager->findBookingById(bookingId);
    if (!booking) return;

    if (actionType == "cancel") {
        CustomConfirmDialog dialog("Xác nhận hủy đặt phòng", QString("Bạn có muốn hủy đơn đặt phòng %1?").arg(QString::fromStdString(bookingId)), false, this);
        if (dialog.exec() == QDialog::Accepted && dialog.isConfirmed()) {
            std::string errMsg;
            if (m_manager->cancelBooking(bookingId, errMsg)) {
                refreshData();
                CustomSuccessDialog("Đơn đặt phòng đã được hủy.", this).exec();
            } else {
                QMessageBox::critical(this, "Lỗi hủy đặt phòng", QString::fromStdString(errMsg));
            }
        }
    } else if (actionType == "delete") {
        CustomConfirmDialog dialog("Xác nhận xóa đặt phòng", QString("Bạn có chắc chắn muốn xóa vĩnh viễn đơn đặt phòng %1 khỏi hệ thống?").arg(QString::fromStdString(bookingId)), true, this);
        if (dialog.exec() == QDialog::Accepted && dialog.isConfirmed()) {
            std::string errMsg;
            if (m_manager->deleteBooking(bookingId, errMsg)) {
                refreshData();
                CustomSuccessDialog("Đơn đặt phòng đã được xóa hoàn toàn.", this).exec();
            } else {
                QMessageBox::critical(this, "Lỗi xóa đặt phòng", QString::fromStdString(errMsg));
            }
        }
    } else if (actionType == "edit") {
        ReservationDialog dialog(m_manager, this);
        dialog.setEditBooking(bookingId);
        if (dialog.exec() == QDialog::Accepted) {
            auto customer = booking->getCustomer();
            if (customer) {
                customer->setName(dialog.getCustomerName().toStdString());
                customer->setPhoneNumber(dialog.getCustomerPhone().toStdString());
            }

            booking->setCheckInDate(dialog.getCheckInDate().toStdString());
            booking->setCheckOutDate(dialog.getCheckOutDate().toStdString());
            
            auto room = m_manager->findRoomByNumber(dialog.getRoomNumber().toStdString());
            if (room) {
                booking->setRoom(room);
            }

            refreshData();
            CustomSuccessDialog("Cập nhật thông tin đặt phòng thành công.", this).exec();
        }
    } else if (actionType == "checkout") {
        CustomConfirmDialog dialog("Xác nhận trả phòng", QString("Tiến hành trả phòng và thanh toán cho giao dịch %1?").arg(QString::fromStdString(bookingId)), false, this);
        if (dialog.exec() == QDialog::Accepted && dialog.isConfirmed()) {
            // 1. Calculate nights
            QDate checkIn = QDate::fromString(QString::fromStdString(booking->getCheckInDate()), Qt::ISODate);
            QDate today = QDate::currentDate();
            
            // Adjust checkout date to today for early/on-time checkout
            booking->setCheckOutDate(today.toString("yyyy-MM-dd").toStdString());

            int nights = checkIn.daysTo(today);
            if (nights <= 0) nights = 1; // Minimum charge 1 night

            // 2. Generate and create Invoice
            std::string invoiceId = m_manager->nextInvoiceId();
            double taxRate = 0.1; // 10% VAT
            std::string todayStr = today.toString("yyyy-MM-dd").toStdString();

            std::string errMsg;
            if (m_manager->createInvoice(invoiceId, bookingId, taxRate, nights, todayStr, errMsg)) {
                auto invoice = m_manager->findInvoiceById(invoiceId);
                if (invoice) {
                    std::string details = invoice->generateInvoiceDetails();
                    InvoiceDialog dialog(QString::fromStdString(details), this);
                    dialog.exec();
                }
                refreshData();
            } else {
                QMessageBox::critical(this, "Lỗi xuất hóa đơn", QString::fromStdString(errMsg));
            }
        }
    }
}
