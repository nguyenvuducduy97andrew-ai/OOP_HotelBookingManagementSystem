#include "ReservationsPageWidget.h"
#include "ReservationDialog.h"
#include "Customer.h"
#include "Room.h"
#include "Invoice.h"
#include "DataManager.h"
#include "CustomConfirmDialog.h"
#include "InvoiceDialog.h"
#include "SchedulePickerDialog.h"
#include "DialogWindowBehavior.h"
#include "SearchFieldUi.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDateTimeEdit>
#include <QDate>
#include <QDateTime>
#include <QTime>
#include <QLocale>
#include <QDebug>
#include <QFrame>
#include <QLabel>
#include <QFormLayout>
#include <QStringList>
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QMouseEvent>
#include <QEvent>
#include <algorithm>

namespace {
class FullRowHoverDelegate : public QStyledItemDelegate {
public:
    explicit FullRowHoverDelegate(QTableWidget* table)
        : QStyledItemDelegate(table), m_table(table)
    {
        m_table->viewport()->installEventFilter(this);
        m_table->setMouseTracking(true);
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override
    {
        QStyleOptionViewItem rowOption(option);
        rowOption.state &= ~QStyle::State_HasFocus;
        if (index.row() == m_hoveredRow) {
            rowOption.state |= QStyle::State_MouseOver;
        } else {
            rowOption.state &= ~QStyle::State_MouseOver;
        }
        QStyledItemDelegate::paint(painter, rowOption, index);
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == m_table->viewport()) {
            int hoveredRow = m_hoveredRow;
            if (event->type() == QEvent::MouseMove) {
                const auto* mouseEvent = static_cast<QMouseEvent*>(event);
                const QModelIndex index = m_table->indexAt(mouseEvent->position().toPoint());
                hoveredRow = index.isValid() ? index.row() : -1;
            } else if (event->type() == QEvent::Leave) {
                hoveredRow = -1;
            }
            if (hoveredRow != m_hoveredRow) {
                m_hoveredRow = hoveredRow;
                m_table->viewport()->update();
            }
        }
        return QStyledItemDelegate::eventFilter(watched, event);
    }

private:
    QTableWidget* m_table;
    int m_hoveredRow = -1;
};

// Modified: Format checkout totals locally with comma thousands separators for readable VND values.
QString formatMoney(double value)
{
    return QLocale(QLocale::English, QLocale::UnitedStates).toString(value, 'f', 0);
}

QString displayBookingTime(const std::string& timestamp, const std::string& legacyDate)
{
    const QDateTime value = QDateTime::fromString(QString::fromStdString(timestamp), Qt::ISODateWithMs);
    if (value.isValid()) {
        return value.toString("yyyy-MM-dd HH:mm");
    }
    return QString::fromStdString(legacyDate);
}

enum class ReservationFeedbackTone { Information, Success, Error };

// Modified: Use one rounded, word-wrapped feedback dialog for Reservation operations instead of system message boxes.
void showReservationFeedback(
    QWidget* parent,
    const QString& title,
    const QString& subtitle,
    const QString& message,
    ReservationFeedbackTone tone)
{
    const bool isError = tone == ReservationFeedbackTone::Error;
    const bool isSuccess = tone == ReservationFeedbackTone::Success;
    const QString accentBackground = isError ? "#FEF2F2" : (isSuccess ? "#ECFDF5" : "#EFF6FF");
    const QString accentBorder = isError ? "#FECACA" : (isSuccess ? "#A7F3D0" : "#BFDBFE");
    const QString buttonColor = isError ? "#DC2626" : (isSuccess ? "#059669" : "#3B58FF");

    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.setModal(true);
    dialog.setMinimumWidth(480);
    // Modified: Render feedback in an opaque custom dialog shell so it matches the application instead of inheriting the system-dark title bar.
    dialog.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    dialog.setStyleSheet(QString(
        "QDialog { background: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 16px; }"
        "QFrame#feedbackHeader { background:#FFFFFF; border:none; border-bottom:1px solid #E7EDF7; border-top-left-radius:15px; border-top-right-radius:15px; }"
        "QLabel#feedbackWindowTitle { color:#1B3F83; font-size:13px; font-weight:800; background:transparent; border:none; }"
        "QLabel#feedbackHeaderIcon { color:%3; font-size:16px; background:transparent; border:none; }"
        "QPushButton#feedbackWindowClose { background:transparent; color:#64748B; border:none; border-radius:8px; font-size:20px; }"
        "QPushButton#feedbackWindowClose:hover { background:#F1F5F9; color:#1B3F83; }"
        "QFrame#feedbackBody { background:#FFFFFF; border:none; border-bottom-left-radius:15px; border-bottom-right-radius:15px; }"
        "QFrame#feedbackPanel { background: %1; border: 1px solid %2; border-radius: 14px; }"
        // Modified: Prevent the application-wide QLabel background from creating white rectangles inside colored feedback panels.
        "QFrame#feedbackPanel QLabel { background: transparent; border: none; }"
        "QLabel#feedbackTitle { color: #1B3F83; font-size: 17px; font-weight: 800; }"
        "QLabel#feedbackSubtitle { color: #64748B; font-size: 12px; font-weight: 600; }"
        "QLabel#feedbackMessage { color: #2B3674; font-size: 13px; }"
        "QPushButton#feedbackClose { background: %3; color: #FFFFFF; border: none; border-radius: 8px;"
        " font-weight: 700; min-width: 96px; padding: 8px 18px; }"
        "QPushButton#feedbackClose:hover { background: #2B7BFF; }")
        .arg(accentBackground, accentBorder, buttonColor));

    auto* layout = new QVBoxLayout(&dialog);
    layout->setSizeConstraint(QLayout::SetFixedSize);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    auto* header = new QFrame(&dialog);
    header->setObjectName("feedbackHeader");
    header->setFixedHeight(48);
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(18, 0, 10, 0);
    auto* headerIcon = new QLabel(isError ? "⚠" : (isSuccess ? "✓" : "ⓘ"), header);
    headerIcon->setObjectName("feedbackHeaderIcon");
    auto* headerTitle = new QLabel(title, header);
    headerTitle->setObjectName("feedbackWindowTitle");
    auto* headerClose = new QPushButton(header);
    headerClose->setObjectName("feedbackWindowClose");
    styleDialogCloseButton(headerClose);
    headerLayout->addWidget(headerIcon);
    headerLayout->addSpacing(8);
    headerLayout->addWidget(headerTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(headerClose);
    QObject::connect(headerClose, &QPushButton::clicked, &dialog, &QDialog::reject);
    enableDialogHeaderDrag(&dialog, header);
    layout->addWidget(header);
    auto* body = new QFrame(&dialog);
    body->setObjectName("feedbackBody");
    auto* bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(24, 20, 24, 20);
    bodyLayout->setSpacing(14);
    auto* panel = new QFrame(&dialog);
    panel->setObjectName("feedbackPanel");
    auto* panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(18, 16, 18, 16);
    panelLayout->setSpacing(6);
    auto* titleLabel = new QLabel(title, panel);
    titleLabel->setObjectName("feedbackTitle");
    auto* subtitleLabel = new QLabel(subtitle, panel);
    subtitleLabel->setObjectName("feedbackSubtitle");
    subtitleLabel->setWordWrap(true);
    auto* messageLabel = new QLabel(message, panel);
    messageLabel->setObjectName("feedbackMessage");
    messageLabel->setWordWrap(true);
    panelLayout->addWidget(titleLabel);
    if (!subtitle.isEmpty()) panelLayout->addWidget(subtitleLabel);
    panelLayout->addWidget(messageLabel);
    bodyLayout->addWidget(panel);
    auto* closeButton = new QPushButton("Close", &dialog);
    closeButton->setObjectName("feedbackClose");
    QObject::connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    bodyLayout->addWidget(closeButton, 0, Qt::AlignCenter);
    layout->addWidget(body);
    lockDialogToWorkingArea(&dialog, layout->sizeHint());
    dialog.exec();
}

QString bookingStateText(BookingState state)
{
    switch (state) {
    case BookingState::UPCOMING: return "Upcoming";
    case BookingState::ACTIVE: return "Active";
    case BookingState::COMPLETED: return "Completed";
    case BookingState::CANCELLED: return "Cancelled";
    case BookingState::NO_SHOW: return "Cancelled";
    }
    return "Unknown";
}

// Modified: Present read-only reservation facts in the same rounded dialog language used by Reservation confirmations.
void showReservationDetails(QWidget* parent, const std::shared_ptr<Booking>& booking, BookingState state)
{
    if (!booking) return;
    const auto customer = booking->getCustomer();
    const auto room = booking->getRoom();
    QDialog dialog(parent);
    dialog.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    dialog.setWindowTitle("Reservation details");
    dialog.setModal(true);
    dialog.setMinimumWidth(560);
    dialog.setWindowFlags(dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    dialog.setStyleSheet(
        "QDialog { background: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 16px; }"
        "QFrame#detailsPanel { background: #F8FAFC; border: 1px solid #DCE6F5; border-radius: 14px; }"
        "QFrame#detailsPanel QLabel { background: transparent; border: none; }"
        "QLabel#detailsTitle { color: #1B3F83; font-size: 18px; font-weight: 800; }"
        "QLabel#detailsSubtitle { color: #64748B; font-size: 12px; }"
        "QLabel#detailsKey { color: #64748B; font-size: 12px; font-weight: 700; }"
        "QLabel#detailsValue { color: #2B3674; font-size: 13px; }"
        "QPushButton { background: #3B58FF; color: #FFFFFF; border: none; border-radius: 8px;"
        " font-weight: 700; min-width: 96px; padding: 8px 18px; }"
        "QPushButton:hover { background: #4F6BFF; }");
    auto* layout = new QVBoxLayout(&dialog);
    layout->setSizeConstraint(QLayout::SetFixedSize);
    layout->setContentsMargins(24, 22, 24, 20);
    layout->setSpacing(12);
    auto* title = new QLabel("Reservation details", &dialog);
    title->setObjectName("detailsTitle");
    auto* subtitle = new QLabel("Read-only operational record for this reservation.", &dialog);
    subtitle->setObjectName("detailsSubtitle");
    layout->addWidget(title);
    layout->addWidget(subtitle);
    auto* panel = new QFrame(&dialog);
    panel->setObjectName("detailsPanel");
    auto* form = new QFormLayout(panel);
    form->setContentsMargins(18, 16, 18, 16);
    form->setHorizontalSpacing(24);
    form->setVerticalSpacing(10);
    const auto addRow = [form, &dialog](const QString& label, const QString& value) {
        auto* key = new QLabel(label, &dialog);
        key->setObjectName("detailsKey");
        auto* field = new QLabel(value, &dialog);
        field->setObjectName("detailsValue");
        field->setWordWrap(true);
        form->addRow(key, field);
    };
    addRow("Booking ID", QString::fromStdString(booking->getBookingId()));
    addRow("Status", bookingStateText(state));
    addRow("Guest", customer ? QString::fromStdString(customer->getName()) : "Unavailable");
    addRow("Customer ID", customer ? QString::fromStdString(customer->getDocumentNumber()) : "Unavailable");
    addRow("Phone", customer ? QString::fromStdString(customer->getPhoneNumber()) : "Unavailable");
    addRow("Room", room ? QString::fromStdString(room->getRoomNumber()) : "Unavailable");
    addRow("Planned stay", displayBookingTime(booking->getPlannedCheckInAt(), booking->getCheckInDate())
        + "  →  " + displayBookingTime(booking->getPlannedCheckOutAt(), booking->getEffectiveCheckOutDate()));
    if (booking->isCheckedIn()) {
        addRow("Actual check-in", displayBookingTime(booking->getActualCheckInAt(), booking->getActualCheckInDate()));
    }
    if (booking->isCheckedOut()) {
        addRow("Actual check-out", displayBookingTime(booking->getActualCheckOutAt(), booking->getActualCheckOutDate()));
    }
    if ((state == BookingState::CANCELLED || state == BookingState::NO_SHOW)
        && !booking->getCancellationReason().empty()) {
        addRow("Cancellation reason", QString::fromStdString(booking->getCancellationReason()));
    }
    layout->addWidget(panel);
    auto* closeButton = new QPushButton("Close", &dialog);
    QObject::connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    layout->addWidget(closeButton, 0, Qt::AlignCenter);
    // Modified: Read-only details are fixed to their content and cannot exceed the parent operational window.
    lockDialogToWorkingArea(&dialog, layout->sizeHint());
    dialog.exec();
}

bool requestReservationReason(
    QWidget* parent,
    const QString& title,
    const QString& explanation,
    const QString& confirmText,
    QString& reason)
{
    QDialog dialog(parent);
    dialog.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    dialog.setWindowTitle(title);
    dialog.setModal(true);
    dialog.setMinimumWidth(380);
    dialog.setStyleSheet(
        "QDialog { background: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 16px; }"
        "QLabel { color: #2B3674; font-size: 13px; }"
        "QLabel#dialogTitle { color: #1B3F83; font-size: 17px; font-weight: 800; }"
        "QLabel#dialogSubtitle { color: #64748B; font-size: 12px; }"
        "QLineEdit { color: #2B3674; background: #F8FAFC; border: 1px solid #CBD5E1;"
        " border-radius: 8px; padding: 9px 10px; font-size: 13px; }"
        "QLineEdit:focus { border: 1px solid #3B58FF; }"
        "QPushButton { min-width: 88px; padding: 8px 14px; border-radius: 8px; font-weight: 700; }"
        "QPushButton#cancelButton { color: #475569; background: #FFFFFF; border: 1px solid #CBD5E1; }"
        "QPushButton#confirmButton { color: #FFFFFF; background: #3B58FF; border: none; }"
        "QPushButton#confirmButton:hover { background: #4F6BFF; }"
    );

    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(22, 20, 22, 18);
    layout->setSpacing(12);

    // Modified: Give the cancellation form the same title-and-subtitle structure as all Reservation operation dialogs.
    auto* titleLabel = new QLabel(title, &dialog);
    titleLabel->setObjectName("dialogTitle");
    layout->addWidget(titleLabel);
    auto* message = new QLabel(explanation, &dialog);
    message->setObjectName("dialogSubtitle");
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
    lockDialogToWorkingArea(&dialog, layout->sizeHint());
    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    reason = reasonInput->text().trimmed();
    return true;
}

bool requestCheckoutPayment(QWidget* parent, double total, QString& method, double& amount)
{
    QDialog dialog(parent);
    dialog.setWindowTitle("Record checkout payment");
    dialog.setModal(true);
    dialog.setMinimumWidth(420);
    dialog.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    // Modified: Give the checkout-payment dialog explicit colors so inherited page styles cannot render its controls white on white.
    dialog.setStyleSheet(
        "QDialog { background: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 16px; }"
        "QLabel { color: #2B3674; font-size: 13px; font-weight: 600; }"
        "QLabel#dialogTitle { color: #1B3F83; font-size: 17px; font-weight: 800; }"
        "QLabel#dialogSubtitle { color: #64748B; font-size: 12px; font-weight: 500; }"
        "QComboBox, QDoubleSpinBox { color: #2B3674; background: #F4F7FE;"
        " border: 1px solid #D9E2F2; border-radius: 8px; padding: 7px 10px; min-height: 22px; }"
        "QComboBox:focus, QDoubleSpinBox:focus { border: 1px solid #3B58FF; }"
        "QComboBox QAbstractItemView { background: #FFFFFF; color: #1B2559; "
        " selection-color: #FFFFFF; selection-background-color: #3B58FF; }"
        "QPushButton { min-width: 96px; padding: 8px 14px; border-radius: 8px; font-weight: 700; }"
        "QPushButton#checkoutPaymentCancel { color: #2B3674; background: #E9EDF7; border: none; }"
        "QPushButton#checkoutPaymentConfirm { color: #FFFFFF; background: #3B58FF; border: none; }"
        "QPushButton#checkoutPaymentConfirm:hover { background: #4F6BFF; }"
    );
    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(24, 22, 24, 20);
    layout->setSpacing(10);
    // Modified: Explain the required checkout payment before staff enter a method and amount.
    auto* titleLabel = new QLabel("Record checkout payment", &dialog);
    titleLabel->setObjectName("dialogTitle");
    auto* subtitleLabel = new QLabel("A payment entry is required before the stay can be checked out and invoiced.", &dialog);
    subtitleLabel->setObjectName("dialogSubtitle");
    subtitleLabel->setWordWrap(true);
    layout->addWidget(titleLabel);
    layout->addWidget(subtitleLabel);
    // Modified: show checkout currency values with comma grouping while preserving the raw payment amount.
    layout->addWidget(new QLabel(QString("Invoice total: %1 VND").arg(formatMoney(total)), &dialog));
    auto* methodBox = new QComboBox(&dialog);
    methodBox->addItems({"Cash", "Card", "Bank transfer", "Other"});
    auto* amountSpin = new QDoubleSpinBox(&dialog);
    amountSpin->setRange(1.0, total);
    amountSpin->setDecimals(0);
    amountSpin->setValue(total);
    amountSpin->setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
    amountSpin->setGroupSeparatorShown(true);
    amountSpin->setSuffix(" VND");
    layout->addWidget(new QLabel("Payment method:", &dialog));
    layout->addWidget(methodBox);
    layout->addWidget(new QLabel("Amount received:", &dialog));
    layout->addWidget(amountSpin);
    auto* buttons = new QHBoxLayout();
    auto* cancel = new QPushButton("Cancel", &dialog);
    auto* confirm = new QPushButton("Record payment", &dialog);
    cancel->setObjectName("checkoutPaymentCancel");
    confirm->setObjectName("checkoutPaymentConfirm");
    buttons->addStretch(); buttons->addWidget(cancel); buttons->addWidget(confirm); layout->addLayout(buttons);
    QObject::connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(confirm, &QPushButton::clicked, &dialog, &QDialog::accept);
    // Modified: Lock the payment form to its designed content size so staff cannot accidentally stretch a short operational dialog.
    lockDialogToWorkingArea(&dialog, layout->sizeHint());
    if (dialog.exec() != QDialog::Accepted) return false;
    method = methodBox->currentText();
    amount = amountSpin->value();
    return !method.isEmpty() && amount > 0.0;
}

bool requestStayExtension(QWidget* parent, HotelManager* manager, const std::shared_ptr<Booking>& booking,
                          const QDateTime& currentPlannedEnd, QDateTime& extendedPlannedEnd)
{
    if (!manager || !booking || !currentPlannedEnd.isValid()) {
        return false;
    }

    QDateTime earliest = currentPlannedEnd.addSecs(60 * 60);
    const QDateTime now = QDateTime::currentDateTime();
    QDateTime nextWholeHour(now.date(), QTime(now.time().hour(), 0));
    if (nextWholeHour <= now) {
        nextWholeHour = nextWholeHour.addSecs(60 * 60);
    }
    if (earliest < nextWholeHour) {
        earliest = nextWholeHour;
    }

    QDateTime actualCheckIn = QDateTime::fromString(
        QString::fromStdString(booking->getActualCheckInAt()), Qt::ISODateWithMs);
    if (!actualCheckIn.isValid()) {
        actualCheckIn = QDateTime::fromString(QString::fromStdString(booking->getPlannedCheckInAt()), Qt::ISODateWithMs);
    }
    const std::string roomNumber = booking->getRoom() ? booking->getRoom()->getRoomNumber() : std::string{};
    const std::string bookingId = booking->getBookingId();
    const auto availabilityPredicate = [manager, roomNumber, bookingId, earliest](const QDateTime&, const QDateTime& end) {
        if (end < earliest || roomNumber.empty()) {
            return false;
        }
        std::string error;
        const auto rooms = manager->getAvailableRoomsForPeriod(
            earliest.toString(Qt::ISODateWithMs).toStdString(), end.toString(Qt::ISODateWithMs).toStdString(), error, bookingId);
        return std::any_of(rooms.cbegin(), rooms.cend(), [&roomNumber](const std::shared_ptr<Room>& room) {
            return room && room->getRoomNumber() == roomNumber;
        });
    };
    // Modified: Use the shared date-and-hour schedule dialog for extension while locking actual check-in and exposing only planned check-out slots.
    SchedulePickerDialog picker(actualCheckIn, earliest, availabilityPredicate, parent, true);
    picker.setWindowTitle("Extend active stay");
    if (auto* pickerTitle = picker.findChild<QLabel*>("pickerTitle")) {
        pickerTitle->setText("Extend active stay");
    }
    if (auto* applyButton = picker.findChild<QPushButton*>("applyButton")) {
        applyButton->setText("Extend stay");
    }
    const auto pickerHints = picker.findChildren<QLabel*>("pickerHint");
    if (!pickerHints.isEmpty()) {
        pickerHints.first()->setText("Actual check-in is fixed. Choose a later planned check-out from an available whole-hour slot.");
    }
    if (picker.exec() != QDialog::Accepted) {
        return false;
    }
    extendedPlannedEnd = picker.selectedCheckOut();
    return extendedPlannedEnd > currentPlannedEnd;
}

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
        QDateEdit:focus, QComboBox:focus, QLineEdit:focus, QSpinBox:focus {
            border: 1px solid #3B58FF;
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
            selection-background-color: #3B58FF;
            selection-color: #FFFFFF;
            border: 1px solid #E9EDF7;
        }
        QPushButton#btnAddBooking {
            background-color: #3B58FF;
            color: #FFFFFF;
            font-weight: bold;
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
            selection-background-color: #EAF2FF;
            selection-color: #2B3674;
        }
        QTableWidget::item {
            padding: 4px 10px;
        }
        QTableWidget::item:hover {
            background-color: #F8FAFF;
            color: #2B3674;
            border: none;
        }
        QTableWidget::item:selected,
        QTableWidget::item:selected:active,
        QTableWidget::item:selected:!active,
        QTableWidget::item:selected:hover {
            background-color: #EAF2FF;
            color: #2B3674;
            border: none;
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

    auto* btnAddBooking = new QPushButton("+ New Reservation", this);
    btnAddBooking->setObjectName("btnAddBooking");
    btnAddBooking->setCursor(Qt::PointingHandCursor);
    connect(btnAddBooking, &QPushButton::clicked, this, [this]() {
        openReservationDialog();
    });
    headerRow->addWidget(pageTitle);
    headerRow->addStretch();
    headerRow->addWidget(btnAddBooking);
    mainLayout->addLayout(headerRow);

    // Filter Row
    auto* filterRow = new QHBoxLayout();
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setObjectName("searchEdit");
    m_searchEdit->setPlaceholderText("Search by guest name, room number...");
    addSearchIcon(m_searchEdit);

    m_statusCombo = new QComboBox(this);
    m_statusCombo->setObjectName("statusCombo");
    // Modified: Keep completed reservations accessible for invoice and read-only audit actions without mixing their workflow with active stays.
    m_statusCombo->addItems({"All statuses", "Upcoming", "Active", "Completed", "Cancelled"});

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
    // Modified: Reserve room for labelled action buttons so staff can understand each operation without an icon legend.
    m_tableWidget->setColumnWidth(8, 340);
    // Modified: Left-align headers so their text does not visually clip against both section borders.
    m_tableWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableWidget->setItemDelegate(new FullRowHoverDelegate(m_tableWidget));
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

void ReservationsPageWidget::startNewReservationForRoom(const QString& roomNumber, const QDateTime& checkIn,
                                                        const QDateTime& checkOut, int adults, int children) {
    // Modified: Retain Room Status date-and-hour choices so the new reservation opens with the exact availability schedule.
    openReservationDialog(roomNumber, checkIn, checkOut, adults, children);
}

void ReservationsPageWidget::openReservationDialog(const QString& preselectedRoomNumber,
                                                    const QDateTime& initialCheckIn, const QDateTime& initialCheckOut,
                                                    int initialAdults, int initialChildren) {
    const auto previewRoom = preselectedRoomNumber.isEmpty() || !m_manager
        ? nullptr : m_manager->findRoomByNumber(preselectedRoomNumber.toStdString());
    ReservationDialog dialog(m_manager, this, previewRoom);
    if (initialCheckIn.isValid() && initialCheckOut.isValid()) {
        dialog.setInitialScheduleAt(initialCheckIn, initialCheckOut, initialAdults, initialChildren);
    }
    if (!preselectedRoomNumber.isEmpty() && !dialog.selectRoom(preselectedRoomNumber.toStdString())) {
        showReservationFeedback(this, "Room unavailable", "Choose another room or schedule.",
            "The selected room is no longer available for the reservation dates you chose.",
            ReservationFeedbackTone::Information);
        emit roomStatusBookingCancelled();
        return;
    }
    if (dialog.exec() == QDialog::Accepted) {
        std::string custId = dialog.getCustomerId().toStdString();
        std::string name = dialog.getCustomerName().toStdString();
        std::string phone = dialog.getCustomerPhone().toStdString();
        std::string roomNum = dialog.getRoomNumber().toStdString();
        std::string plannedCheckInAt = dialog.getPlannedCheckInAt().toStdString();
        std::string plannedCheckOutAt = dialog.getPlannedCheckOutAt().toStdString();

        std::string errMsg;
        const bool bookingCreated = dialog.usesExistingCustomer()
            ? m_manager->createBookingAt(custId, roomNum, plannedCheckInAt, plannedCheckOutAt,
                                         dialog.getAdultCount(), dialog.getChildCount(), errMsg)
            : m_manager->createBookingForNewCustomer(custId, name, phone, roomNum,
                                                     plannedCheckInAt, plannedCheckOutAt,
                                                     dialog.getAdultCount(), dialog.getChildCount(), errMsg);

        // Modified: Persist a new customer only together with its validated reservation so a failed room booking cannot leave an orphan customer record.
        if (bookingCreated) {
            if (!DataManager::getInstance().commitChanges(*m_manager)) {
                refreshData();
                showReservationFeedback(this, "Reservation not saved", "Your previous data remains unchanged.",
                    "The reservation could not be saved. Please try again after reviewing the booking details.",
                    ReservationFeedbackTone::Error);
                return;
            }
            refreshData();
            emit bookingChanged();
            showReservationFeedback(this, "Reservation created", "The room and schedule are now reserved.",
                "The new reservation was saved successfully.", ReservationFeedbackTone::Success);
        } else {
            showReservationFeedback(this, "Reservation could not be created", "Review the room, schedule, and customer details.",
                QString::fromStdString(errMsg), ReservationFeedbackTone::Error);
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
        // Apply Status Filter
        if (m_statusFilterIndex > 0) {
            bool matches = false;
            if (m_statusFilterIndex == 1 && state == BookingState::UPCOMING) matches = true;
            else if (m_statusFilterIndex == 2 && state == BookingState::ACTIVE) matches = true;
            else if (m_statusFilterIndex == 3 && state == BookingState::COMPLETED) matches = true;
            else if (m_statusFilterIndex == 4
                && (state == BookingState::CANCELLED || state == BookingState::NO_SHOW)) matches = true;

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
        m_tableWidget->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(customer->getDocumentNumber())));
        m_tableWidget->setItem(row, 2, new QTableWidgetItem(custName));
        m_tableWidget->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(customer->getPhoneNumber())));
        m_tableWidget->setItem(row, 4, new QTableWidgetItem(roomNum));
        m_tableWidget->setItem(row, 5, new QTableWidgetItem(
            displayBookingTime(booking->getPlannedCheckInAt(), booking->getCheckInDate())));
        m_tableWidget->setItem(row, 6, new QTableWidgetItem(
            displayBookingTime(booking->getPlannedCheckOutAt(), booking->getEffectiveCheckOutDate())));

        // Status Item & color
        auto* statusItem = new QTableWidgetItem();
        if (state == BookingState::UPCOMING) {
            statusItem->setText("📅 Upcoming");
            statusItem->setForeground(QColor("#3B58FF"));
        } else if (state == BookingState::ACTIVE) {
            statusItem->setText("🛏 Active");
            statusItem->setForeground(QColor("#D97706"));
        } else if (state == BookingState::COMPLETED) {
            statusItem->setText("✔ Completed");
            statusItem->setForeground(QColor("#05CD99"));
        } else if (state == BookingState::CANCELLED || state == BookingState::NO_SHOW) {
            statusItem->setText("✖ Cancelled");
            statusItem->setForeground(QColor("#EF4444"));
        }
        m_tableWidget->setItem(row, 7, statusItem);

        // Action Column - Multiple buttons layout container
        auto* actionContainer = new QWidget(m_tableWidget);
        auto* actionLayout = new QHBoxLayout(actionContainer);
        actionLayout->setContentsMargins(5, 0, 6, 0);
        actionLayout->setSpacing(6);
        actionLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        actionLayout->setSizeConstraint(QLayout::SetMinimumSize);

        const auto addActionButton = [&](const QString& text, const QString& actionType,
                                         const QString& style, const QString& tooltip,
                                         bool enabled = true) {
            auto* button = new QPushButton(text, actionContainer);
            button->setCursor(enabled ? Qt::PointingHandCursor : Qt::ArrowCursor);
            button->setMinimumHeight(30);
            button->setToolTip(tooltip);
            button->setEnabled(enabled);
            button->setStyleSheet(style +
                " QPushButton:disabled { background-color: #F1F5F9; color: #94A3B8; border: 1px solid #E2E8F0; }");
            button->setProperty("bookingId", bId);
            button->setProperty("actionType", actionType);
            connect(button, &QPushButton::clicked, this, &ReservationsPageWidget::onTableActionClicked);
            actionLayout->addWidget(button);
        };

        if (state == BookingState::ACTIVE) {
            const QDateTime plannedEnd = QDateTime::fromString(
                QString::fromStdString(booking->getPlannedCheckOutAt()), Qt::ISODateWithMs);
            const bool mayEarlyCheckout = plannedEnd.isValid() && QDateTime::currentDateTime() < plannedEnd;
            // Modified: Label each active-stay operation directly; early checkout remains distinct from the normal checkout path.
            addActionButton("💳 Check out", "checkout",
                "QPushButton { background-color: #ECFDF5; color: #065F46; border: 1px solid #A7F3D0; border-radius: 8px; font-weight: 700; padding: 0 9px; }",
                "Record payment, check out the guest, and issue an invoice");
            addActionButton("↗ Early check-out", "earlycheckout",
                "QPushButton { background-color: #FFF7ED; color: #C2410C; border: 1px solid #FED7AA; border-radius: 8px; font-weight: 700; padding: 0 9px; }",
                mayEarlyCheckout ? "Check out before the planned departure time" : "Available only before the planned departure time",
                mayEarlyCheckout);
            // Modified: Keep controlled active-stay extension available without allowing unrestricted reservation editing.
            addActionButton("⏱ Extend", "extend",
                "QPushButton { background-color: #EFF6FF; color: #1D4ED8; border: 1px solid #BFDBFE; border-radius: 8px; font-weight: 700; padding: 0 9px; }",
                "Extend the planned stay if the room remains available");

        } else if (state == BookingState::UPCOMING) {
            QDateTime plannedCheckInAt = QDateTime::fromString(QString::fromStdString(booking->getPlannedCheckInAt()), Qt::ISODateWithMs);
            QDateTime plannedCheckOutAt = QDateTime::fromString(QString::fromStdString(booking->getPlannedCheckOutAt()), Qt::ISODateWithMs);
            if (!plannedCheckInAt.isValid()) {
                plannedCheckInAt = QDateTime(QDate::fromString(QString::fromStdString(booking->getCheckInDate()), Qt::ISODate), QTime(0, 0));
            }
            if (!plannedCheckOutAt.isValid()) {
                plannedCheckOutAt = QDateTime(QDate::fromString(QString::fromStdString(booking->getCheckOutDate()), Qt::ISODate), QTime(0, 0));
            }
            const QDateTime now = QDateTime::currentDateTime();
            const bool mayCheckIn = plannedCheckInAt.isValid() && plannedCheckOutAt.isValid()
                && now.date() >= plannedCheckInAt.date() && now < plannedCheckOutAt;
            // Modified: Display every upcoming-reservation action with a verb, while disabling check-in until its valid operational window.
            addActionButton("🛎 Check in", "checkin",
                "QPushButton { background-color: #EFF6FF; color: #1D4ED8; border: 1px solid #BFDBFE; border-radius: 8px; font-weight: 700; padding: 0 9px; }",
                mayCheckIn ? "Record the guest's actual arrival now; same-day early check-in is allowed when the room is ready"
                           : QString("Check-in opens on %1. Same-day early check-in is allowed, but arrival cannot be recorded on an earlier calendar date.")
                                 .arg(plannedCheckInAt.toString("dd MMM yyyy")),
                mayCheckIn);
            addActionButton("✖ Cancel", "cancel",
                "QPushButton { background-color: #FFFBEB; color: #92400E; border: 1px solid #FDE68A; border-radius: 8px; font-weight: 700; padding: 0 9px; }",
                "Cancel this unarrived reservation and record a reason");
            addActionButton("✎ Edit", "edit",
                "QPushButton { background-color: #E9EFFF; color: #1E40AF; border: 1px solid #C3D4FF; border-radius: 8px; font-weight: 700; padding: 0 9px; }",
                "Change the planned schedule or selected room");

        } else if (state == BookingState::COMPLETED) {
            // Modified: Retain auditable completed reservations in the list with explicit invoice and detail actions.
            addActionButton("▣ Invoice", "invoice",
                "QPushButton { background-color: #ECFDF5; color: #065F46; border: 1px solid #A7F3D0; border-radius: 8px; font-weight: 700; padding: 0 9px; }",
                "Open the issued invoice for this completed reservation");
            addActionButton("ⓘ Details", "details",
                "QPushButton { background-color: #F4F7FE; color: #2B3674; border: 1px solid #DCE6F5; border-radius: 8px; font-weight: 700; padding: 0 9px; }",
                "View the read-only reservation record");

        } else if (state == BookingState::CANCELLED || state == BookingState::NO_SHOW) {
            // Modified: Render legacy no-show records as cancelled records so staff never receive a separate no-show action.
            addActionButton("ⓘ Details", "details",
                "QPushButton { background-color: #F4F7FE; color: #2B3674; border: 1px solid #DCE6F5; border-radius: 8px; font-weight: 700; padding: 0 9px; }",
                "View the read-only cancellation record and reason");

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
    performBookingAction(QString::fromStdString(bookingId), actionType);
}

void ReservationsPageWidget::performBookingAction(const QString& requestedBookingId, const QString& actionType) {
    const std::string bookingId = requestedBookingId.toStdString();

    auto booking = m_manager->findBookingById(bookingId);
    if (!booking || booking->isDeleted()) return;

    if (actionType == "checkin") {
        CustomConfirmDialog dialog("Confirm check-in", QString("Check in reservation %1 now?").arg(QString::fromStdString(bookingId)), false, this);
        if (dialog.exec() == QDialog::Accepted && dialog.isConfirmed()) {
            std::string errMsg;
            // Modified: Record the exact arrival instant so the actual stay and invoice start from staff-confirmed check-in.
            const std::string now = QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toStdString();
            if (!m_manager->checkInBookingAt(bookingId, now, errMsg)) {
                showReservationFeedback(this, "Check-in could not be completed", "The reservation remains upcoming.",
                    QString::fromStdString(errMsg), ReservationFeedbackTone::Error);
                return;
            }
            if (!DataManager::getInstance().commitChanges(*m_manager)) {
                refreshData();
                showReservationFeedback(this, "Check-in was not saved", "The reservation was restored to its previous state.",
                    "Please retry the check-in after verifying the reservation.", ReservationFeedbackTone::Error);
                return;
            }
            refreshData();
            emit bookingChanged();
            showReservationFeedback(this, "Guest checked in", "Actual stay time is now being recorded.",
                "The reservation is active and the room is occupied.", ReservationFeedbackTone::Success);
        }
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
                showReservationFeedback(this, "Cancellation reason required", "A reason is needed for the audit record.",
                    "Enter a short cancellation reason before confirming this reservation.", ReservationFeedbackTone::Information);
                return;
            }
            std::string errMsg;
            if (m_manager->cancelBooking(bookingId, reason.toStdString(), errMsg)) {
                if (!DataManager::getInstance().commitChanges(*m_manager)) {
                    refreshData();
                    showReservationFeedback(this, "Cancellation was not saved", "The reservation was restored to its previous state.",
                        "Please try again after reviewing the cancellation details.", ReservationFeedbackTone::Error);
                    return;
                }
                refreshData();
                emit bookingChanged();
                showReservationFeedback(this, "Reservation cancelled", "The room is available for future reservations.",
                    "The cancellation reason was saved with the reservation record.", ReservationFeedbackTone::Success);
            } else {
                showReservationFeedback(this, "Reservation could not be cancelled", "The reservation remains unchanged.",
                    QString::fromStdString(errMsg), ReservationFeedbackTone::Error);
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
                showReservationFeedback(this, "Customer verification failed", "The reservation has not been changed.",
                    QString::fromStdString(errMsg), ReservationFeedbackTone::Error);
                return;
            }

            // Modified: Persist the edited timestamp range through the same interval engine used for new reservations.
            if (!m_manager->updateBookingAt(
                    bookingId,
                    custId,
                    dialog.getRoomNumber().toStdString(),
                    dialog.getPlannedCheckInAt().toStdString(),
                    dialog.getPlannedCheckOutAt().toStdString(),
                    dialog.getAdultCount(),
                    dialog.getChildCount(),
                    errMsg)) {
                showReservationFeedback(this, "Reservation could not be updated", "The previous schedule remains in effect.",
                    QString::fromStdString(errMsg), ReservationFeedbackTone::Error);
                return;
            }

            if (!DataManager::getInstance().commitChanges(*m_manager)) {
                refreshData();
                showReservationFeedback(this, "Reservation update was not saved", "The previous reservation state was restored.",
                    "Please retry after reviewing the new schedule and room availability.", ReservationFeedbackTone::Error);
                return;
            }

            refreshData();
            emit bookingChanged();
            showReservationFeedback(this, "Reservation updated", "The revised room and schedule are now active.",
                "Reservation information was saved successfully.", ReservationFeedbackTone::Success);
        }
    } else if (actionType == "extend") {
        QDateTime currentPlannedEnd = QDateTime::fromString(
            QString::fromStdString(booking->getPlannedCheckOutAt()), Qt::ISODateWithMs);
        if (!currentPlannedEnd.isValid()) {
            currentPlannedEnd = QDateTime(
                QDate::fromString(QString::fromStdString(booking->getCheckOutDate()), Qt::ISODate), QTime(0, 0));
        }
        QDateTime extendedPlannedEnd;
        if (!requestStayExtension(this, m_manager, booking, currentPlannedEnd, extendedPlannedEnd)) {
            return;
        }

        std::string errMsg;
        if (!m_manager->extendActiveBookingAt(
                bookingId, extendedPlannedEnd.toString(Qt::ISODateWithMs).toStdString(), errMsg)) {
            showReservationFeedback(this, "Stay could not be extended", "The existing planned check-out remains in effect.",
                QString::fromStdString(errMsg), ReservationFeedbackTone::Error);
            return;
        }
        if (!DataManager::getInstance().commitChanges(*m_manager)) {
            refreshData();
            showReservationFeedback(this, "Stay extension was not saved", "The reservation was restored to its previous schedule.",
                "Please retry after reviewing the room's next reservation and cleaning buffer.", ReservationFeedbackTone::Error);
            return;
        }
        // Modified: Persist a successful extension before refreshing so the active booking remains consistent with availability and cleaning planning.
        refreshData();
        emit bookingChanged();
        showReservationFeedback(this, "Stay extended", "The new planned check-out has been saved.",
            "Room availability and the following cleaning buffer were recalculated.", ReservationFeedbackTone::Success);
    } else if (actionType == "invoice") {
        const auto invoice = m_manager->findInvoiceForBooking(bookingId);
        if (!invoice) {
            showReservationFeedback(this, "Invoice unavailable", "This completed reservation has no issued invoice record.",
                "Please ask a manager to review the financial record before making any changes.", ReservationFeedbackTone::Information);
            return;
        }
        InvoiceDialog invoiceDialog(QString::fromStdString(invoice->generateInvoiceDetails()), this);
        invoiceDialog.exec();
    } else if (actionType == "details") {
        showReservationDetails(this, booking, m_manager->getBookingState(*booking));
    } else if (actionType == "checkout" || actionType == "earlycheckout") {
        const QDateTime checkoutAt = QDateTime::currentDateTime();
        const bool earlyCheckout = actionType == "earlycheckout";
        const QDateTime plannedEnd = QDateTime::fromString(
            QString::fromStdString(booking->getPlannedCheckOutAt()), Qt::ISODateWithMs);
        if (earlyCheckout && (!plannedEnd.isValid() || checkoutAt >= plannedEnd)) {
            showReservationFeedback(this, "Early check-out is unavailable", "The planned departure time has already been reached.",
                "Use Check out to complete this active stay.", ReservationFeedbackTone::Information);
            return;
        }
        // Modified: State the checkout consequence before payment so early departures cannot be confused with a simple status change.
        QString checkoutMessage = QString("Record payment, check out reservation %1, and issue its invoice?")
            .arg(QString::fromStdString(bookingId));
        if (earlyCheckout) {
            checkoutMessage += "\n\nThis is an early check-out. Billing will use the actual time of departure, and the room will enter its cleaning period immediately.";
        }
        const auto warnings = m_manager->getCheckoutConflictWarnings(
            bookingId, checkoutAt.toString(Qt::ISODateWithMs).toStdString());
        if (!warnings.empty()) {
            QStringList warningLines;
            for (const std::string& warning : warnings) {
                warningLines.append(QString::fromStdString(warning));
            }
            // Modified: Present warnings as a short operational brief so staff can understand the consequence before proceeding to payment.
            checkoutMessage += "\n\nWhat needs attention:\n• " + warningLines.join("\n• ")
                + "\n\nWhat to do: coordinate the next arrival if the room will not be ready.";
        }
        CustomConfirmDialog dialog(
            warnings.empty() ? (earlyCheckout ? "Confirm early check-out" : "Confirm check-out") : "Check-out needs attention",
            checkoutMessage,
            !warnings.empty(),
            this,
            warnings.empty() ? "Continue" : "Continue to payment",
            warnings.empty() ? "Cancel" : "Go back",
            !warnings.empty());
        if (dialog.exec() == QDialog::Accepted && dialog.isConfirmed()) {
            QDateTime actualCheckIn = QDateTime::fromString(QString::fromStdString(booking->getActualCheckInAt()), Qt::ISODateWithMs);
            if (!actualCheckIn.isValid()) {
                actualCheckIn = QDateTime(
                    QDate::fromString(QString::fromStdString(booking->getActualCheckInDate()), Qt::ISODate), QTime(0, 0));
            }
            const qint64 actualSeconds = actualCheckIn.isValid() ? actualCheckIn.secsTo(checkoutAt) : 0;
            if (actualSeconds <= 0) {
                showReservationFeedback(this, "Check-out could not be completed", "Actual departure must follow actual arrival.",
                    "Wait until the guest has a positive recorded stay duration, then try again.", ReservationFeedbackTone::Error);
                return;
            }
            // Modified: Quote the checkout payment from exact elapsed time, rounded half-up to billable hours.
            const int billableHours = std::max(1, static_cast<int>((actualSeconds + 30 * 60) / (60 * 60)));
            const double hourlyRate = booking->getQuotedHourlyRate() > 0.0
                ? booking->getQuotedHourlyRate() : booking->getQuotedUnitPrice();
            const double invoiceTotal = billableHours * hourlyRate * (1.0 + booking->getQuotedTaxRate());
            QString paymentMethod;
            double paymentAmount = 0.0;
            // Modified: Require a real payment entry before checkout can complete and issue an invoice.
            if (!requestCheckoutPayment(this, invoiceTotal, paymentMethod, paymentAmount)) {
                return;
            }

            std::string errMsg;
            if (!m_manager->completeBookingAt(bookingId, checkoutAt.toString(Qt::ISODateWithMs).toStdString(), errMsg)) {
                showReservationFeedback(this, "Check-out could not be completed", "The guest remains checked in and the room status is unchanged.",
                    QString::fromStdString(errMsg), ReservationFeedbackTone::Error);
                return;
            }

            // Modified: Stage checkout and invoice together before one atomic persistence commit.
            std::string invoiceId = m_manager->nextInvoiceId();

            const std::string todayStr = checkoutAt.date().toString(Qt::ISODate).toStdString();
            if (!m_manager->createInvoice(invoiceId, bookingId, todayStr, paymentMethod.toStdString(),
                                          paymentAmount, todayStr, errMsg)) {
                DataManager::getInstance().restoreLastSavedState(*m_manager);
                refreshData();
                showReservationFeedback(this, "Invoice could not be issued", "The checkout was rolled back to keep the stay and invoice consistent.",
                    QString::fromStdString(errMsg), ReservationFeedbackTone::Error);
                return;
            }

            // Modified: Commit checkout and invoice as one database snapshot or restore the prior state.
            if (!DataManager::getInstance().commitChanges(*m_manager)) {
                refreshData();
                showReservationFeedback(this, "Check-out was not saved", "The previous database state was restored.",
                    "The guest remains checked in until the checkout and invoice can be saved together.", ReservationFeedbackTone::Error);
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
        }
    }
}
