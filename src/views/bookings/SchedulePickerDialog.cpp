#include "SchedulePickerDialog.h"
#include "DialogWindowBehavior.h"

#include <QButtonGroup>
#include <QCalendarWidget>
#include <QColor>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QStyle>
#include <QTextCharFormat>
#include <QTime>
#include <QVBoxLayout>
#include <utility>

namespace {
constexpr int kHoursPerDay = 24;
constexpr int kMinimumBookingSeconds = 60 * 60;

QDateTime wholeHour(const QDateTime& value)
{
    return QDateTime(value.date(), QTime(value.time().hour(), 0));
}
}

SchedulePickerDialog::SchedulePickerDialog(const QDateTime& initialCheckIn,
                                           const QDateTime& initialCheckOut,
                                           AvailabilityPredicate availabilityPredicate,
                                           QWidget* parent,
                                           bool checkOutOnly,
                                           bool startInCheckOutMode,
                                           Purpose purpose)
    : QDialog(parent),
      m_availabilityPredicate(std::move(availabilityPredicate)),
      m_checkIn(purpose == Purpose::BookingStay && checkOutOnly
                    ? initialCheckIn
                    : QDateTime(initialCheckIn.date(), purpose == Purpose::BookingStay ? QTime(initialCheckIn.time().hour(), 0) : QTime(0, 0))),
      m_checkOut(QDateTime(initialCheckOut.date(), purpose == Purpose::BookingStay ? QTime(initialCheckOut.time().hour(), 0) : QTime(0, 0))),
      m_checkOutOnly(checkOutOnly),
      m_purpose(purpose)
{
    if (isDateOnlyPurpose()) {
        // Modified: Reuse the booking-room calendar shell for full-day maintenance and reporting without imposing hourly booking rules.
        if (!m_checkIn.isValid() || (!allowsPastDates() && m_checkIn.date() < QDate::currentDate())) {
            m_checkIn = QDateTime(QDate::currentDate(), QTime(0, 0));
        }
        if (!m_checkOut.isValid() || m_checkOut.date() < m_checkIn.date()) {
            m_checkOut = m_checkIn;
        }
    } else {
        if (!m_checkIn.isValid() || (!m_checkOutOnly && m_checkIn < QDateTime(QDate::currentDate(), QTime(0, 0)))) {
            m_checkIn = wholeHour(QDateTime::currentDateTime().addSecs(kMinimumBookingSeconds));
        }
        if (!m_checkOut.isValid() || m_checkOut < m_checkIn.addSecs(kMinimumBookingSeconds)) {
            m_checkOut = m_checkIn.addSecs(kMinimumBookingSeconds);
        }
    }

    const bool maintenancePurpose = m_purpose == Purpose::MaintenanceRange;
    const bool reportingPurpose = m_purpose == Purpose::ReportingPeriod;
    const QString dialogTitle = maintenancePurpose
        ? QStringLiteral("Choose maintenance dates")
        : reportingPurpose ? QStringLiteral("Choose reporting period") : QStringLiteral("Choose dates & times");
    setWindowTitle(dialogTitle);
    setModal(true);
    // Modified: Keep the complete schedule dialog at its A4-like working size so dates, hours and actions remain visible together.
    lockDialogToWorkingArea(this, isDateOnlyPurpose() ? QSize(720, 510) : QSize(720, 700));
    // Modified: Use an opaque custom header to avoid the platform-dark title bar without allowing background content to bleed through.
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setStyleSheet(R"(
        QDialog { background: #FAFCFF; border: 2px solid #93C5FD; border-radius: 18px; }
        QFrame#dialogHeader { background: #FFFFFF; border: none; border-bottom: 1px solid #E7EDF7;border-top-left-radius: 13px; border-top-right-radius: 13px; }
        QLabel#pickerTitle { color: #163779; font-size: 19px; font-weight: 800; background: transparent; border: none; }
        QPushButton#windowClose { background: #FFF7F7; color: #B91C1C; border: 1px solid #ff7c7c; border-radius: 8px; padding: 0px; font-family: "Segoe UI"; font-size: 12px; font-weight: 800; }
        QPushButton#windowClose:hover { background: #f59e9e; color: #FFFFFF; }
        QLabel { color: #2B3674; background: transparent; border: none; font-size: 13px; font-weight: 600; }
        QLabel#pickerHint { color: #6B7FA8; font-weight: 500; }
        QLabel#pickerHint[error="true"] { color: #B45309; }
        QPushButton { border-radius: 9px; padding: 8px 12px; font-weight: 700; }
        QPushButton#modeButton:checked { background: #3B58FF; color: #FFFFFF; border: 1px solid #3B58FF; }
        QPushButton#modeButton:!checked { background: #F4F7FE; color: #2B3674; border: 1px solid #D9E2F2; }
        QPushButton#hourButton { background: #F4F7FE; color: #2B3674; border: 1px solid #D9E2F2; min-width: 0px; padding: 8px 6px; }
        QPushButton#hourButton:hover:enabled { background: #DBEAFE; border-color: #93C5FD; }
        QPushButton#hourButton:checked { background: #3B58FF; color: #FFFFFF; border-color: #3B58FF; }
        QPushButton#hourButton:disabled { color: #A3AED0; background: #F1F5F9; border-color: #E2E8F0; }
        QPushButton#applyButton { background: #3B58FF; color: #FFFFFF; border: 1px solid #3B58FF; min-width: 124px; }
        QPushButton#applyButton:hover { background: #4F6BFF; }
        QPushButton#cancelButton { background: #E9EDF7; color: #2B3674; border: none; min-width: 100px; }
        QCalendarWidget QWidget { color: #2B3674; background: #FFFFFF; }
        QCalendarWidget QAbstractItemView:enabled { selection-background-color: #3B58FF; selection-color: #FFFFFF; }
        QCalendarWidget QAbstractItemView:disabled { color: #A3AED0; }
        QCalendarWidget QToolButton { color: #2B3674; background: transparent; }
        QCalendarWidget QMenu { background: #FFFFFF; color: #2B3674; }
        QCalendarWidget QHeaderView::section { background: #DDEBFF; color: #174A9A; font-weight: 800; border: none; padding: 5px; }
    )");

    auto* rootLayout = new QVBoxLayout(this);
    // Modified: Reserve a pale-blue outer rim around the complete dialog, including the custom title bar.
    rootLayout->setContentsMargins(3, 3, 3, 3);
    rootLayout->setSpacing(0);

    auto* header = new QFrame(this);
    header->setObjectName("dialogHeader");
    header->setFixedHeight(48);
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(22, 0, 12, 0);
    auto* scheduleIcon = new QLabel("◷", header);
    scheduleIcon->setStyleSheet("color:#3B58FF; font-size:18px; background:transparent; border:none;");
    auto* title = new QLabel(dialogTitle, header);
    title->setObjectName("pickerTitle");
    auto* close = new QPushButton("×",header);
    close->setObjectName("windowClose");
    styleDialogCloseButton(close);
    headerLayout->addWidget(scheduleIcon);
    headerLayout->addSpacing(8);
    headerLayout->addWidget(title);
    headerLayout->addStretch();
    headerLayout->addWidget(close);
    connect(close, &QPushButton::clicked, this, &QDialog::reject);
    enableDialogHeaderDrag(this, header);
    rootLayout->addWidget(header);
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setMinimumHeight(0);
    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* content = new QWidget(scrollArea);
    content->setMinimumHeight(0);
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(6, 4, 6, 6);
    layout->setSpacing(8);

    const QString subtitleText = maintenancePurpose
        ? QStringLiteral("Choose the first and last full day when this room will be unavailable for maintenance.")
        : reportingPurpose
            ? QStringLiteral("Choose the inclusive start and end dates used by the report and invoice filters.")
            : QStringLiteral("Select a check-in date and time first, then switch to Check-out to complete the stay.");
    auto* subtitle = new QLabel(subtitleText, this);
    subtitle->setObjectName("pickerHint");
    subtitle->setWordWrap(true);
    layout->addWidget(subtitle);

    auto* modeRow = new QHBoxLayout();
    auto* checkInMode = new QPushButton(isDateOnlyPurpose() ? "Start date" : "Check-in", this);
    auto* checkOutMode = new QPushButton(isDateOnlyPurpose() ? "End date" : "Check-out", this);
    checkInMode->setObjectName("modeButton");
    checkOutMode->setObjectName("modeButton");
    checkInMode->setCheckable(true);
    checkOutMode->setCheckable(true);
    m_modeGroup = new QButtonGroup(this);
    m_modeGroup->setExclusive(true);
    m_modeGroup->addButton(checkInMode, 0);
    m_modeGroup->addButton(checkOutMode, 1);
    (startInCheckOutMode ? checkOutMode : checkInMode)->setChecked(true);
    if (m_checkOutOnly) {
        // Modified: Active-stay extension reuses the shared picker but locks actual arrival, leaving only planned check-out editable.
        checkInMode->hide();
        checkOutMode->setChecked(true);
    }
    modeRow->addWidget(checkInMode);
    modeRow->addWidget(checkOutMode);
    modeRow->addStretch();

    m_calendar = new QCalendarWidget(this);
    // Modified: Constrain the calendar itself to a centered, self-contained grid so all seven weekdays are visible without horizontal scrolling.
    m_calendar->setFixedSize(500, 276);
    if (!allowsPastDates()) {
        m_calendar->setMinimumDate(QDate::currentDate());
    }
    m_calendar->setSelectedDate(m_modeGroup->checkedId() == 0 ? m_checkIn.date() : m_checkOut.date());
    // Modified: Keep week numbers available to Qt internally but remove the non-actionable column from reservation scheduling.
    m_calendar->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    layout->addWidget(m_calendar, 0, Qt::AlignHCenter);
    layout->addLayout(modeRow);

    auto* hoursLabel = new QLabel("Available hour slots", this);
    hoursLabel->setVisible(!isDateOnlyPurpose());
    layout->addWidget(hoursLabel);
    // Modified: Keep all 24 hourly choices inside the same compact visual width as the calendar.
    auto* hourGridContainer = new QWidget(content);
    hourGridContainer->setFixedWidth(500);
    hourGridContainer->setVisible(!isDateOnlyPurpose());
    auto* hourGrid = new QGridLayout(hourGridContainer);
    hourGrid->setContentsMargins(0, 0, 0, 0);
    hourGrid->setHorizontalSpacing(6);
    hourGrid->setVerticalSpacing(4);
    for (int hour = 0; hour < kHoursPerDay; ++hour) {
        auto* button = new QPushButton(QString("%1:00").arg(hour, 2, 10, QLatin1Char('0')), this);
        button->setObjectName("hourButton");
        button->setCheckable(true);
        button->setFixedHeight(36);
        // Modified: Let each slot share the compact grid width evenly instead of enforcing a width that makes adjacent slots overlap.
        button->setMinimumWidth(0);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_hourButtons[hour] = button;
        hourGrid->addWidget(button, hour / 6, hour % 6);
        connect(button, &QPushButton::clicked, this, [this, hour]() {
            const QDate date = m_modeGroup->checkedId() == 0 ? m_checkIn.date() : m_checkOut.date();
            if (m_modeGroup->checkedId() == 0) {
                m_checkIn = QDateTime(date, QTime(hour, 0));
                // Modified: A new check-in choice replaces the tentative stay instead of silently preserving a previous check-out date.
                m_checkOut = m_checkIn.addSecs(kMinimumBookingSeconds);
            } else {
                m_checkOut = QDateTime(date, QTime(hour, 0));
            }
            refreshAvailability();
        });
    }
    layout->addWidget(hourGridContainer, 0, Qt::AlignHCenter);

    m_summaryLabel = new QLabel(this);
    m_summaryLabel->setWordWrap(true);
    layout->addWidget(m_summaryLabel);
    m_hintLabel = new QLabel(this);
    m_hintLabel->setObjectName("pickerHint");
    m_hintLabel->setWordWrap(true);
    layout->addWidget(m_hintLabel);

    auto* buttons = new QDialogButtonBox(this);
    auto* cancelButton = buttons->addButton("Cancel", QDialogButtonBox::RejectRole);
    const QString applyText = maintenancePurpose
        ? QStringLiteral("Apply maintenance dates")
        : reportingPurpose ? QStringLiteral("Apply period") : QStringLiteral("Apply schedule");
    auto* applyButton = buttons->addButton(applyText, QDialogButtonBox::AcceptRole);
    cancelButton->setObjectName("cancelButton");
    applyButton->setObjectName("applyButton");
    // Modified: Set footer button colors directly so the action text remains above an opaque, high-contrast button on every platform theme.
    cancelButton->setStyleSheet("QPushButton { background:#E9EDF7; color:#2B3674; border:none; border-radius:9px; padding:8px 16px; font-weight:700; min-width:108px; }");
    applyButton->setStyleSheet("QPushButton { background:#3B58FF; color:#FFFFFF; border:1px solid #3B58FF; border-radius:9px; padding:8px 16px; font-weight:700; min-width:136px; }");
    buttons->setFixedHeight(46);
    buttons->setCenterButtons(false);
    scrollArea->setWidget(content);
    rootLayout->addWidget(scrollArea, 1);
    // Modified: Keep Apply and Cancel outside the scrollable body so neither action can disappear below a short display.
    rootLayout->addWidget(buttons);

    // Modified: One shared picker keeps date, hour and availability feedback consistent across reservation scheduling flows.
    connect(m_calendar, &QCalendarWidget::clicked, this, [this](const QDate& date) {
        if (!isDayAvailable(date)) {
            m_calendar->setSelectedDate(m_modeGroup->checkedId() == 0 ? m_checkIn.date() : m_checkOut.date());
            showHint("This date has no selectable hour for the current schedule.", true);
            return;
        }
        if (m_modeGroup->checkedId() == 0) {
            m_checkIn.setDate(date);
            // Modified: Continue replacing the range start until staff explicitly switches modes; date-only workflows keep a valid one-day range.
            m_checkOut = isDateOnlyPurpose() ? m_checkIn : m_checkIn.addSecs(kMinimumBookingSeconds);
        } else if (date < m_checkIn.date()) {
            showHint(isDateOnlyPurpose()
                         ? "The end date cannot be before the selected start date."
                         : "Check-out cannot be before the selected check-in date.",
                     true);
            m_calendar->setSelectedDate(m_checkOut.date());
            return;
        } else {
            m_checkOut.setDate(date);
        }
        refreshAvailability();
    });
    connect(m_modeGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, [this](int) {
        m_calendar->setSelectedDate(m_modeGroup->checkedId() == 0 ? m_checkIn.date() : m_checkOut.date());
        refreshAvailability();
    });
    connect(m_calendar, &QCalendarWidget::currentPageChanged, this, [this](int, int) { refreshDayAvailability(); });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        if (!isRangeAvailable(m_checkIn, m_checkOut)) {
            showHint(isDateOnlyPurpose()
                         ? "Choose a valid start and end date for this period."
                         : "No room is available for this complete schedule. Choose another date or hour.",
                     true);
            return;
        }
        accept();
    });

    refreshAvailability();
}

QDateTime SchedulePickerDialog::selectedCheckIn() const
{
    return m_checkIn;
}

QDateTime SchedulePickerDialog::selectedCheckOut() const
{
    return m_checkOut;
}

bool SchedulePickerDialog::isRangeAvailable(const QDateTime& checkIn, const QDateTime& checkOut) const
{
    if (isDateOnlyPurpose()) {
        return checkIn.isValid() && checkOut.isValid()
            && (allowsPastDates() || checkIn.date() >= QDate::currentDate())
            && checkOut.date() >= checkIn.date()
            && (!m_availabilityPredicate || m_availabilityPredicate(checkIn, checkOut));
    }
    // Modified: Reject elapsed whole-hour slots in the picker so staff never reaches the save step with a past planned arrival.
    return checkIn.isValid() && (m_checkOutOnly || checkIn >= QDateTime::currentDateTime())
        && checkOut >= checkIn.addSecs(kMinimumBookingSeconds)
        && (!m_availabilityPredicate || m_availabilityPredicate(checkIn, checkOut));
}

bool SchedulePickerDialog::isDayAvailable(const QDate& day) const
{
    const bool chooseCheckIn = m_modeGroup->checkedId() == 0;
    if ((!allowsPastDates() && day < QDate::currentDate()) || (!chooseCheckIn && day < m_checkIn.date())) {
        return false;
    }
    if (isDateOnlyPurpose()) {
        const QDateTime candidateIn = chooseCheckIn ? QDateTime(day, QTime(0, 0)) : m_checkIn;
        const QDateTime candidateOut = chooseCheckIn ? candidateIn : QDateTime(day, QTime(0, 0));
        return isRangeAvailable(candidateIn, candidateOut);
    }
    for (int hour = 0; hour < kHoursPerDay; ++hour) {
        QDateTime candidateIn = m_checkIn;
        QDateTime candidateOut = m_checkOut;
        if (chooseCheckIn) {
            candidateIn = QDateTime(day, QTime(hour, 0));
            // Modified: Check-in availability is evaluated as a fresh minimum stay, never against a stale tentative check-out.
            candidateOut = candidateIn.addSecs(kMinimumBookingSeconds);
        } else {
            candidateOut = QDateTime(day, QTime(hour, 0));
        }
        if (isRangeAvailable(candidateIn, candidateOut)) {
            return true;
        }
    }
    return false;
}

void SchedulePickerDialog::refreshAvailability()
{
    refreshDayAvailability();
    refreshHourAvailability();
    refreshSelectionPresentation();
    updateSummary();
    showHint(isDateOnlyPurpose()
                 ? "Grey dates cannot be selected for this period."
                 : "Grey dates and hour slots have no room available for the selected range.");
}

void SchedulePickerDialog::refreshDayAvailability()
{
    const QDate firstVisible(m_calendar->yearShown(), m_calendar->monthShown(), 1);
    const QDate lastVisible = firstVisible.addMonths(1).addDays(-1);
    const bool chooseCheckIn = m_modeGroup->checkedId() == 0;
    QTextCharFormat available;
    QTextCharFormat unavailable;
    unavailable.setForeground(QColor("#A3AED0"));
    unavailable.setBackground(QColor("#F1F5F9"));
    for (QDate day = firstVisible; day <= lastVisible; day = day.addDays(1)) {
        if ((!allowsPastDates() && day < QDate::currentDate()) || (!chooseCheckIn && day < m_checkIn.date())) {
            m_calendar->setDateTextFormat(day, unavailable);
            continue;
        }
        const bool anyHourAvailable = isDayAvailable(day);
        m_calendar->setDateTextFormat(day, anyHourAvailable ? available : unavailable);
    }
}

void SchedulePickerDialog::refreshHourAvailability()
{
    if (isDateOnlyPurpose()) {
        return;
    }
    const bool chooseCheckIn = m_modeGroup->checkedId() == 0;
    for (int hour = 0; hour < kHoursPerDay; ++hour) {
        QDateTime candidateIn = m_checkIn;
        QDateTime candidateOut = m_checkOut;
        if (chooseCheckIn) {
            candidateIn.setTime(QTime(hour, 0));
            // Modified: Changing the check-in hour also resets the provisional check-out to one hour later.
            candidateOut = candidateIn.addSecs(kMinimumBookingSeconds);
        } else {
            candidateOut.setTime(QTime(hour, 0));
        }
        const bool enabled = isRangeAvailable(candidateIn, candidateOut);
        m_hourButtons[hour]->setEnabled(enabled);
        const int selectedHour = chooseCheckIn ? m_checkIn.time().hour() : m_checkOut.time().hour();
        m_hourButtons[hour]->setChecked(enabled && hour == selectedHour);
    }
}

void SchedulePickerDialog::refreshSelectionPresentation()
{
    QTextCharFormat rangeFormat;
    rangeFormat.setBackground(QColor("#DBEAFE"));
    rangeFormat.setForeground(QColor("#1E40AF"));
    for (QDate day = m_checkIn.date(); day <= m_checkOut.date() && day <= m_checkIn.date().addDays(366); day = day.addDays(1)) {
        // Modified: Preserve the unavailable-day treatment inside a selected range so blue range highlighting never suggests a blocked day can be chosen.
        if (isDayAvailable(day)) {
            m_calendar->setDateTextFormat(day, rangeFormat);
        }
    }
    m_calendar->setSelectedDate(m_modeGroup->checkedId() == 0 ? m_checkIn.date() : m_checkOut.date());
}

void SchedulePickerDialog::updateSummary()
{
    if (isDateOnlyPurpose()) {
        const QString prefix = m_purpose == Purpose::MaintenanceRange
            ? QStringLiteral("Maintenance period") : QStringLiteral("Reporting period");
        m_summaryLabel->setText(QString("%1: %2  →  %3 (inclusive)")
            .arg(prefix, m_checkIn.date().toString("dd MMM yyyy"), m_checkOut.date().toString("dd MMM yyyy")));
        return;
    }
    m_summaryLabel->setText(QString("Selected schedule: %1  →  %2\nMinimum reservation duration: one hour.")
        .arg(m_checkIn.toString("dd MMM yyyy, HH:mm"), m_checkOut.toString("dd MMM yyyy, HH:mm")));
}

bool SchedulePickerDialog::isDateOnlyPurpose() const
{
    return m_purpose != Purpose::BookingStay;
}

bool SchedulePickerDialog::allowsPastDates() const
{
    return m_purpose == Purpose::ReportingPeriod;
}

void SchedulePickerDialog::showHint(const QString& message, bool isError)
{
    m_hintLabel->setText(message);
    m_hintLabel->setProperty("error", isError);
    m_hintLabel->style()->unpolish(m_hintLabel);
    m_hintLabel->style()->polish(m_hintLabel);
}
