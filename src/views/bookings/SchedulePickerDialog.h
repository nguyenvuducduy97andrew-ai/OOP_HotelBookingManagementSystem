#pragma once

#include <QDate>
#include <QDateTime>
#include <QDialog>
#include <functional>

class QButtonGroup;
class QCalendarWidget;
class QLabel;
class QPushButton;

class SchedulePickerDialog : public QDialog {
public:
    using AvailabilityPredicate = std::function<bool(const QDateTime&, const QDateTime&)>;

    enum class Purpose {
        BookingStay,
        MaintenanceRange,
        ReportingPeriod
    };

    explicit SchedulePickerDialog(const QDateTime& initialCheckIn,
                                  const QDateTime& initialCheckOut,
                                  AvailabilityPredicate availabilityPredicate,
                                  QWidget* parent = nullptr,
                                  bool checkOutOnly = false,
                                  bool startInCheckOutMode = false,
                                  Purpose purpose = Purpose::BookingStay);

    QDateTime selectedCheckIn() const;
    QDateTime selectedCheckOut() const;

private:
    void refreshAvailability();
    void refreshDayAvailability();
    void refreshHourAvailability();
    void refreshSelectionPresentation();
    void updateSummary();
    bool isRangeAvailable(const QDateTime& checkIn, const QDateTime& checkOut) const;
    bool isDayAvailable(const QDate& day) const;
    bool isDateOnlyPurpose() const;
    bool allowsPastDates() const;
    void showHint(const QString& message, bool isError = false);

    AvailabilityPredicate m_availabilityPredicate;
    QDateTime m_checkIn;
    QDateTime m_checkOut;
    bool m_checkOutOnly = false;
    Purpose m_purpose = Purpose::BookingStay;
    QCalendarWidget* m_calendar;
    QButtonGroup* m_modeGroup;
    QLabel* m_summaryLabel;
    QLabel* m_hintLabel;
    QPushButton* m_hourButtons[24]{};
};
