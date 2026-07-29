#include "ReservationsPageWidget.h"
#include "ReservationDialog.h"
#include "Customer.h"
#include "Room.h"
#include "Invoice.h"
#include "DataManager.h"
#include "CustomSuccessDialog.h"
#include "CustomConfirmDialog.h"
#include "InvoiceDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>
#include <QDialog>
#include <QLineEdit>
#include <QDate>
#include <QDebug>
#include <QFrame>
#include <QLabel>
#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QFontMetrics>
#include <QEvent>

namespace {
bool requestReservationReason(
    QWidget* parent,
    const QString& title,
    const QString& explanation,
    const QString& confirmText,
    QString& reason)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.setModal(true);
    dialog.setMinimumWidth(380);
    dialog.setStyleSheet(
        "QDialog { background: #FFFFFF; }"
        "QLabel { color: #2B3674; font-size: 13px; }"
        "QLineEdit { color: #2B3674; background: #F8FAFC; border: 1px solid #CBD5E1;"
        " border-radius: 8px; padding: 9px 10px; font-size: 13px; }"
        "QLineEdit:focus { border: 1px solid #005BFE; }"
        "QPushButton { min-width: 88px; padding: 8px 14px; border-radius: 8px; font-weight: 700; }"
        "QPushButton#cancelButton { color: #475569; background: #FFFFFF; border: 1px solid #CBD5E1; }"
        "QPushButton#confirmButton { color: #FFFFFF; background: #005BFE; border: none; }"
        "QPushButton#confirmButton:hover { background: #2B7BFF; }"
    );

    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(22, 20, 22, 18);
    layout->setSpacing(12);

    auto* message = new QLabel(explanation, &dialog);
    message->setWordWrap(true);
    layout->addWidget(message);

    auto* reasonLabel = new QLabel("Reason (optional)", &dialog);
    layout->addWidget(reasonLabel);

    auto* reasonInput = new QLineEdit(&dialog);
    reasonInput->setPlaceholderText("Enter a short note for the audit record");
    reasonInput->setClearButtonEnabled(true);
    layout->addWidget(reasonInput);

    auto* buttonRow = new QHBoxLayout();
    buttonRow->addStretch();
    auto* cancelButton = new QPushButton("Cancel", &dialog);
    cancelButton->setObjectName("cancelButton");
    auto* confirmButton = new QPushButton(confirmText, &dialog);
    confirmButton->setObjectName("confirmButton");
    buttonRow->addWidget(cancelButton);
    buttonRow->addWidget(confirmButton);
    layout->addLayout(buttonRow);

    QObject::connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(confirmButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    QObject::connect(reasonInput, &QLineEdit::returnPressed, &dialog, &QDialog::accept);

    reasonInput->setFocus();
    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    reason = reasonInput->text().trimmed();
    return true;
}

class BookingActionTooltipFilter : public QObject
{
public:
    BookingActionTooltipFilter(QWidget* anchor, QString tooltipText, QObject* parent = nullptr)
        : QObject(parent), m_anchor(anchor), m_tooltipText(std::move(tooltipText))
    {
        m_popup = new QFrame(nullptr, Qt::ToolTip | Qt::FramelessWindowHint);
        m_popup->setObjectName("bookingActionTooltipPopup");
        m_popup->setStyleSheet(
            "QFrame#bookingActionTooltipPopup {"
            " background-color: #FFFFFF;"
            " color: #000000;"
            " border: 1px solid #FCA5A5;"
            " border-radius: 6px;"
            "}"
        );

        auto* layout = new QHBoxLayout(m_popup);
        layout->setContentsMargins(10, 6, 10, 6);

        auto* label = new QLabel(m_tooltipText, m_popup);
        label->setStyleSheet("color: #000000; font-size: 12px; font-weight: 600;");
        label->setWordWrap(false);
        layout->addWidget(label);

        const QFontMetrics metrics(label->font());
        const int width = metrics.horizontalAdvance(m_tooltipText) + 24;
        const int height = metrics.height() + 12;
        m_popup->setFixedSize(width, height);
    }

    ~BookingActionTooltipFilter() override
    {
        if (m_popup) {
            m_popup->hide();
            m_popup->deleteLater();
        }
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched != m_anchor) {
            return QObject::eventFilter(watched, event);
        }

        if (event->type() == QEvent::Enter) {
            showPopup();
        } else if (event->type() == QEvent::Leave || event->type() == QEvent::Hide) {
            if (m_popup) {
                m_popup->hide();
            }
        }

        return QObject::eventFilter(watched, event);
    }

private:
    void showPopup()
    {
        if (!m_anchor || !m_popup) {
            return;
        }

        const QScreen* screen = QGuiApplication::screenAt(m_anchor->mapToGlobal(QPoint(0, 0)));
        if (!screen) {
            screen = QGuiApplication::primaryScreen();
        }

        const QRect available = screen ? screen->availableGeometry() : QRect();
        QPoint target = m_anchor->mapToGlobal(QPoint((m_anchor->width() - m_popup->width()) / 2, m_anchor->height() + 6));

        if (target.y() + m_popup->height() > available.bottom()) {
            target.setY(m_anchor->mapToGlobal(QPoint(0, 0)).y() - m_popup->height() - 6);
        }

        if (available.isValid()) {
            const int minX = available.left() + 6;
            const int maxX = available.right() - m_popup->width() - 6;
            if (target.x() < minX) target.setX(minX);
            if (target.x() > maxX) target.setX(maxX);
            if (target.y() < available.top() + 6) target.setY(available.top() + 6);
        }

        m_popup->move(target);
        m_popup->show();
        m_popup->raise();
    }

    QWidget* m_anchor;
    QString m_tooltipText;
    QFrame* m_popup = nullptr;
};
}

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
            padding: 8px 10px;
            text-align: left;
            font-size: 12px;
        }
        QToolTip {
            background-color: #FEF2F2;
            color: #000000;
            border: 1px solid #FCA5A5;
            border-radius: 6px;
            padding: 6px 10px;
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
    auto* pageTitle = new QLabel("Reservation Management", this);
    pageTitle->setObjectName("pageTitle");

    headerRow->addWidget(pageTitle);
    headerRow->addStretch();
    mainLayout->addLayout(headerRow);

    // Legend row (explains the action icons).
    auto* legendRow = new QHBoxLayout();
    legendRow->setSpacing(12);
    legendRow->setAlignment(Qt::AlignLeft);

    auto* legendTitle = new QLabel("Action legend:", this);
    legendTitle->setStyleSheet("font-weight: bold; color: #2B3674; font-size: 11px;");
    legendRow->addWidget(legendTitle);

    auto* legCheckOut = new QLabel("💳 Check Out", this);
    legCheckOut->setStyleSheet("background-color: #ECFDF5; color: #065F46; border: 1px solid #A7F3D0; font-weight: bold; border-radius: 10px; padding: 2px 8px; font-size: 11px;");
    legendRow->addWidget(legCheckOut);

    // Modified: Expose every operational reservation action in the legend using the same color and icon as its table button.
    auto* legCheckIn = new QLabel("🛎 Check in", this);
    legCheckIn->setStyleSheet("background-color: #EFF6FF; color: #1D4ED8; border: 1px solid #BFDBFE; font-weight: bold; border-radius: 10px; padding: 2px 8px; font-size: 11px;");
    legendRow->addWidget(legCheckIn);

    auto* legEdit = new QLabel("🖊 Edit", this);
    legEdit->setStyleSheet("background-color: #E9EFFF; color: #1E40AF; border: 1px solid #C3D4FF; font-weight: bold; border-radius: 10px; padding: 2px 8px; font-size: 11px;");
    legendRow->addWidget(legEdit);

    auto* legCancel = new QLabel("❌ Cancel", this);
    legCancel->setStyleSheet("background-color: #FFFBEB; color: #92400E; border: 1px solid #FDE68A; font-weight: bold; border-radius: 10px; padding: 2px 8px; font-size: 11px;");
    legendRow->addWidget(legCancel);

    auto* legNoShow = new QLabel("⚠ No-show", this);
    legNoShow->setStyleSheet("background-color: #F5F3FF; color: #7E22CE; border: 1px solid #DDD6FE; font-weight: bold; border-radius: 10px; padding: 2px 8px; font-size: 11px;");
    legendRow->addWidget(legNoShow);

    mainLayout->addLayout(legendRow);

    // Filter Row
    auto* filterRow = new QHBoxLayout();
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setObjectName("searchEdit");
    m_searchEdit->setPlaceholderText("Search by guest name, room number...");

    m_statusCombo = new QComboBox(this);
    m_statusCombo->setObjectName("statusCombo");
    m_statusCombo->addItems({"All statuses", "Upcoming", "Active", "Cancelled", "No-show"});

    filterRow->addWidget(m_searchEdit);
    filterRow->addWidget(m_statusCombo);
    filterRow->addStretch();
    mainLayout->addLayout(filterRow);

    // Table Widget
    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setColumnCount(9);
    m_tableWidget->setHorizontalHeaderLabels({
        "Booking ID", "Customer ID", "Customer Name", "Phone Number",
        "Room Number", "Check-in", "Check-out", "Status", "Actions"
    });
    
    for (int i = 0; i < 8; ++i) {
        m_tableWidget->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    }
    m_tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch); // Let Customer Name take remaining space
    m_tableWidget->horizontalHeader()->setSectionResizeMode(8, QHeaderView::Fixed);
    // Modified: Reserve button width plus cell padding so the final action's outer border is never clipped.
    m_tableWidget->setColumnWidth(8, 132);
    // Modified: Left-align headers so their text does not visually clip against both section borders.
    m_tableWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableWidget->verticalHeader()->setDefaultSectionSize(42);
    m_tableWidget->verticalHeader()->setVisible(false);
    mainLayout->addWidget(m_tableWidget);

    // Connects
    connect(m_searchEdit, &QLineEdit::textChanged, this, &ReservationsPageWidget::onSearchChanged);
    connect(m_statusCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ReservationsPageWidget::onFilterStatusChanged);
}

void ReservationsPageWidget::onSearchChanged(const QString& text) {
    m_searchQuery = text.trimmed();
    refreshData();
}

void ReservationsPageWidget::onFilterStatusChanged(int index) {
    m_statusFilterIndex = index;
    refreshData();
}

void ReservationsPageWidget::startNewReservationForRoom(const QString& roomNumber) {
    // Modified: Accept a Room Status selection through a typed navigation boundary instead of duplicating booking logic.
    openReservationDialog(roomNumber);
}

void ReservationsPageWidget::openReservationDialog(const QString& preselectedRoomNumber) {
    ReservationDialog dialog(m_manager, this);
    if (!preselectedRoomNumber.isEmpty() && !dialog.selectRoom(preselectedRoomNumber.toStdString())) {
        QMessageBox::warning(this, "Room unavailable", "The selected room is no longer available for the default reservation dates.");
        emit roomStatusBookingCancelled();
        return;
    }
    if (dialog.exec() == QDialog::Accepted) {
        std::string custId = dialog.getCustomerId().toStdString();
        std::string name = dialog.getCustomerName().toStdString();
        std::string phone = dialog.getCustomerPhone().toStdString();
        std::string roomNum = dialog.getRoomNumber().toStdString();
        std::string checkIn = dialog.getCheckInDate().toStdString();
        std::string checkOut = dialog.getCheckOutDate().toStdString();

        std::string errMsg;
        if (!m_manager->resolveCustomerForBooking(custId, name, phone, errMsg)) {
            QMessageBox::critical(this, "Customer verification error", QString::fromStdString(errMsg));
            if (!preselectedRoomNumber.isEmpty()) emit roomStatusBookingCancelled();
            return;
        }

        if (m_manager->createBooking(custId, roomNum, checkIn, checkOut,
                dialog.getAdultCount(), dialog.getChildCount(), errMsg)) {
            if (!DataManager::getInstance().commitChanges(*m_manager)) {
                refreshData();
                QMessageBox::critical(this, "Save Booking Failed", "The reservation was not saved. The previous database state has been restored.");
                return;
            }
            refreshData();
            emit bookingChanged();
            CustomSuccessDialog("Reservation completed successfully.", this).exec();
        } else {
            QMessageBox::critical(this, "Booking error", QString::fromStdString(errMsg));
            if (!preselectedRoomNumber.isEmpty()) emit roomStatusBookingCancelled();
        }
    } else if (!preselectedRoomNumber.isEmpty()) {
        // Modified: Preserve the Room Status origin so a cancel or back action returns to the selected-room workflow.
        emit roomStatusBookingCancelled();
    }
}

void ReservationsPageWidget::refreshData() {
    m_tableWidget->setRowCount(0);
    if (!m_manager) return;

    int row = 0;
    for (const auto& booking : m_manager->getBookings()) {
        if (!booking || booking->isDeleted()) continue;

        BookingState state = m_manager->getBookingState(*booking);
        // Modified: Move completed bookings to Dashboard History and remove them from the operational table.
        if (state == BookingState::COMPLETED) continue;

        // Apply Status Filter
        if (m_statusFilterIndex > 0) {
            bool matches = false;
            if (m_statusFilterIndex == 1 && state == BookingState::UPCOMING) matches = true;
            else if (m_statusFilterIndex == 2 && state == BookingState::ACTIVE) matches = true;
            else if (m_statusFilterIndex == 3 && state == BookingState::CANCELLED) matches = true;
            else if (m_statusFilterIndex == 4 && state == BookingState::NO_SHOW) matches = true;

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
        m_tableWidget->setItem(row, 6, new QTableWidgetItem(QString::fromStdString(booking->getEffectiveCheckOutDate())));

        // Status Item & color
        auto* statusItem = new QTableWidgetItem();
        if (state == BookingState::UPCOMING) {
            statusItem->setText("📅 Upcoming");
            statusItem->setForeground(QColor("#005BFE"));
        } else if (state == BookingState::ACTIVE) {
            statusItem->setText("🛏 Active");
            statusItem->setForeground(QColor("#D97706"));
        } else if (state == BookingState::COMPLETED) {
            statusItem->setText("✔ Completed");
            statusItem->setForeground(QColor("#05CD99"));
        } else if (state == BookingState::CANCELLED) {
            statusItem->setText("✖ Cancelled");
            statusItem->setForeground(QColor("#EF4444"));
        } else if (state == BookingState::NO_SHOW) {
            statusItem->setText("⚠ No-show");
            statusItem->setForeground(QColor("#9333EA"));
        }
        m_tableWidget->setItem(row, 7, statusItem);

        // Action Column - Multiple buttons layout container
        auto* actionContainer = new QWidget(m_tableWidget);
        auto* actionLayout = new QHBoxLayout(actionContainer);
        actionLayout->setContentsMargins(5, 0, 6, 0);
        actionLayout->setSpacing(5);
        actionLayout->setAlignment(Qt::AlignCenter);
        actionLayout->setSizeConstraint(QLayout::SetMinimumSize);

        auto configureActionButton = [](QPushButton* button, const QString& tooltipText)
        {
            button->setToolTip(QString());
            button->setCursor(Qt::PointingHandCursor);
            button->setMinimumSize(30, 30);
            button->setMaximumSize(30, 30);

            auto* tooltipFilter = new BookingActionTooltipFilter(button, tooltipText, button);
            button->installEventFilter(tooltipFilter);
        };

        if (state == BookingState::ACTIVE) {
            // Check Out
            auto* checkOutBtn = new QPushButton("💳", actionContainer);
            // Modified: Checkout issues an invoice but does not claim that a payment has been collected.
            configureActionButton(checkOutBtn, "Check out & issue invoice");
            checkOutBtn->setStyleSheet("background-color: #ECFDF5; color: #065F46; border: 1px solid #A7F3D0; border-radius: 8px; font-size: 14px; min-width: 30px; max-width: 30px; min-height: 30px; max-height: 30px; padding: 0px;");
            checkOutBtn->setProperty("bookingId", bId);
            checkOutBtn->setProperty("actionType", "checkout");
            connect(checkOutBtn, &QPushButton::clicked, this, &ReservationsPageWidget::onTableActionClicked);
            actionLayout->addWidget(checkOutBtn);

            // Edit
            auto* editBtn = new QPushButton("🖊", actionContainer);
            configureActionButton(editBtn, "Edit reservation");
            editBtn->setStyleSheet("background-color: #E9EFFF; color: #1E40AF; border: 1px solid #C3D4FF; border-radius: 8px; font-size: 14px; min-width: 30px; max-width: 30px; min-height: 30px; max-height: 30px; padding: 0px;");
            editBtn->setProperty("bookingId", bId);
            editBtn->setProperty("actionType", "edit");
            connect(editBtn, &QPushButton::clicked, this, &ReservationsPageWidget::onTableActionClicked);
            actionLayout->addWidget(editBtn);

        } else if (state == BookingState::UPCOMING) {
            const QDate today = QDate::currentDate();
            const QDate plannedCheckIn = QDate::fromString(QString::fromStdString(booking->getCheckInDate()), Qt::ISODate);
            const QDate plannedCheckOut = QDate::fromString(QString::fromStdString(booking->getCheckOutDate()), Qt::ISODate);
            if (plannedCheckIn.isValid() && plannedCheckOut.isValid()
                && today >= plannedCheckIn && today < plannedCheckOut) {
                // Modified: Give staff an explicit operational check-in action instead of activating stays by date.
                auto* checkInBtn = new QPushButton("🛎", actionContainer);
                configureActionButton(checkInBtn, "Check in guest");
                checkInBtn->setStyleSheet("background-color: #EFF6FF; color: #1D4ED8; border: 1px solid #BFDBFE; border-radius: 8px; font-size: 14px; min-width: 30px; max-width: 30px; min-height: 30px; max-height: 30px; padding: 0px;");
                checkInBtn->setProperty("bookingId", bId);
                checkInBtn->setProperty("actionType", "checkin");
                connect(checkInBtn, &QPushButton::clicked, this, &ReservationsPageWidget::onTableActionClicked);
                actionLayout->addWidget(checkInBtn);
            }
            if (plannedCheckIn.isValid() && today >= plannedCheckIn) {
                auto* noShowBtn = new QPushButton("⚠", actionContainer);
                configureActionButton(noShowBtn, "Mark no-show");
                noShowBtn->setStyleSheet("background-color: #F5F3FF; color: #7E22CE; border: 1px solid #DDD6FE; border-radius: 8px; font-size: 14px; min-width: 30px; max-width: 30px; min-height: 30px; max-height: 30px; padding: 0px;");
                noShowBtn->setProperty("bookingId", bId);
                noShowBtn->setProperty("actionType", "noshow");
                connect(noShowBtn, &QPushButton::clicked, this, &ReservationsPageWidget::onTableActionClicked);
                actionLayout->addWidget(noShowBtn);
            } else {
                // Cancel
                auto* cancelBtn = new QPushButton("❌", actionContainer);
                configureActionButton(cancelBtn, "Cancel reservation");
                cancelBtn->setStyleSheet("background-color: #FFFBEB; color: #92400E; border: 1px solid #FDE68A; border-radius: 8px; font-size: 14px; min-width: 30px; max-width: 30px; min-height: 30px; max-height: 30px; padding: 0px;");
                cancelBtn->setProperty("bookingId", bId);
                cancelBtn->setProperty("actionType", "cancel");
                connect(cancelBtn, &QPushButton::clicked, this, &ReservationsPageWidget::onTableActionClicked);
                actionLayout->addWidget(cancelBtn);
            }

            // Edit
            auto* editBtn = new QPushButton("🖊", actionContainer);
            configureActionButton(editBtn, "Edit reservation");
            editBtn->setStyleSheet("background-color: #E9EFFF; color: #1E40AF; border: 1px solid #C3D4FF; border-radius: 8px; font-size: 14px; min-width: 30px; max-width: 30px; min-height: 30px; max-height: 30px; padding: 0px;");
            editBtn->setProperty("bookingId", bId);
            editBtn->setProperty("actionType", "edit");
            connect(editBtn, &QPushButton::clicked, this, &ReservationsPageWidget::onTableActionClicked);
            actionLayout->addWidget(editBtn);

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
    if (!booking || booking->isDeleted()) return;

    if (actionType == "checkin") {
        CustomConfirmDialog dialog("Confirm check-in", QString("Check in reservation %1 now?").arg(QString::fromStdString(bookingId)), false, this);
        if (dialog.exec() == QDialog::Accepted && dialog.isConfirmed()) {
            std::string errMsg;
            const std::string today = QDate::currentDate().toString(Qt::ISODate).toStdString();
            if (!m_manager->checkInBooking(bookingId, today, errMsg)) {
                QMessageBox::critical(this, "Check-in error", QString::fromStdString(errMsg));
                return;
            }
            if (!DataManager::getInstance().commitChanges(*m_manager)) {
                refreshData();
                QMessageBox::critical(this, "Save Check-in Failed", "The check-in was not saved. The previous database state has been restored.");
                return;
            }
            refreshData();
            emit bookingChanged();
            CustomSuccessDialog("Guest checked in successfully.", this).exec();
        }
    } else if (actionType == "noshow") {
        QString reason;
        // Modified: Use an explicitly styled dialog because the global theme can hide native QInputDialog labels and buttons.
        if (!requestReservationReason(this, "Mark no-show",
                                      "Record that the guest did not arrive. This releases the room for future availability.",
                                      "Mark no-show", reason)) {
            return;
        }
        std::string errMsg;
        if (!m_manager->markNoShow(bookingId, reason.toStdString(), errMsg)) {
            QMessageBox::critical(this, "No-show error", QString::fromStdString(errMsg));
            return;
        }
        if (!DataManager::getInstance().commitChanges(*m_manager)) {
            refreshData();
            QMessageBox::critical(this, "Save No-show Failed", "The no-show record was not saved. The previous database state has been restored.");
            return;
        }
        refreshData();
        emit bookingChanged();
        CustomSuccessDialog("Reservation marked as no-show.", this).exec();
    } else if (actionType == "cancel") {
        CustomConfirmDialog dialog("Confirm cancel reservation", QString("Do you want to cancel reservation %1?").arg(QString::fromStdString(bookingId)), false, this);
        if (dialog.exec() == QDialog::Accepted && dialog.isConfirmed()) {
            QString reason;
            if (!requestReservationReason(this, "Cancellation reason",
                                          "Add a reason so the cancellation remains traceable in the audit record.",
                                          "Confirm cancellation", reason)) {
                return;
            }
            if (reason.isEmpty()) {
                QMessageBox::warning(this, "Cancellation reason required", "Enter a cancellation reason to preserve the audit record.");
                return;
            }
            std::string errMsg;
            if (m_manager->cancelBooking(bookingId, reason.toStdString(), errMsg)) {
                if (!DataManager::getInstance().commitChanges(*m_manager)) {
                    refreshData();
                    QMessageBox::critical(this, "Save Cancellation Failed", "The cancellation was not saved. The previous database state has been restored.");
                    return;
                }
                refreshData();
                emit bookingChanged();
                CustomSuccessDialog("Reservation has been canceled.", this).exec();
            } else {
                QMessageBox::critical(this, "Cancel reservation error", QString::fromStdString(errMsg));
            }
        }
    } else if (actionType == "edit") {
        ReservationDialog dialog(m_manager, this);
        dialog.setEditBooking(bookingId);
        if (dialog.exec() == QDialog::Accepted) {
            std::string errMsg;
            const std::string custId = dialog.getCustomerId().toStdString();
            const std::string name = dialog.getCustomerName().toStdString();
            const std::string phone = dialog.getCustomerPhone().toStdString();

            if (!m_manager->resolveCustomerForBooking(custId, name, phone, errMsg)) {
                QMessageBox::critical(this, "Customer verification error", QString::fromStdString(errMsg));
                return;
            }

            if (!m_manager->updateBooking(
                    bookingId,
                    custId,
                    dialog.getRoomNumber().toStdString(),
                    dialog.getCheckInDate().toStdString(),
                    dialog.getCheckOutDate().toStdString(),
                    dialog.getAdultCount(),
                    dialog.getChildCount(),
                    errMsg)) {
                QMessageBox::critical(this, "Update reservation error", QString::fromStdString(errMsg));
                return;
            }

            if (!DataManager::getInstance().commitChanges(*m_manager)) {
                refreshData();
                QMessageBox::critical(this, "Save Booking Failed", "The reservation changes were not saved. The previous database state has been restored.");
                return;
            }

            refreshData();
            emit bookingChanged();
            CustomSuccessDialog("Reservation information updated successfully.", this).exec();
        }
    } else if (actionType == "checkout") {
        CustomConfirmDialog dialog("Confirm check-out", QString("Proceed with check-out and issue an invoice for reservation %1?").arg(QString::fromStdString(bookingId)), false, this);
        if (dialog.exec() == QDialog::Accepted && dialog.isConfirmed()) {
            QDate today = QDate::currentDate();
            std::string todayStr = today.toString("yyyy-MM-dd").toStdString();

            std::string errMsg;
            if (!m_manager->completeBooking(bookingId, todayStr, errMsg)) {
                QMessageBox::critical(this, "Check-out error", QString::fromStdString(errMsg));
                return;
            }

            // Modified: Stage checkout and invoice together before one atomic persistence commit.
            std::string invoiceId = m_manager->nextInvoiceId();

            if (!m_manager->createInvoice(invoiceId, bookingId, todayStr, errMsg)) {
                DataManager::getInstance().restoreLastSavedState(*m_manager);
                refreshData();
                QMessageBox::critical(this, "Invoice generation error", QString::fromStdString(errMsg));
                return;
            }

            // Modified: Commit checkout and invoice as one database snapshot or restore the prior state.
            if (!DataManager::getInstance().commitChanges(*m_manager)) {
                refreshData();
                QMessageBox::critical(this, "Check-out Save Failed", "Check-out and invoice were not saved. The previous database state has been restored.");
                return;
            }

            auto invoice = m_manager->findInvoiceById(invoiceId);
            refreshData();
            emit bookingCompleted();
            emit bookingChanged();

            if (invoice) {
                InvoiceDialog invoiceDialog(QString::fromStdString(invoice->generateInvoiceDetails()), this);
                invoiceDialog.exec();
            }
            CustomSuccessDialog("Check-out and invoice saved successfully.", this).exec();
        }
    }
}
