#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QDateTime>
#include "HotelManager.h"

class ReservationsPageWidget : public QWidget {
    Q_OBJECT

public:
    explicit ReservationsPageWidget(HotelManager* manager, QWidget *parent = nullptr);

    void refreshData();
    void startNewReservationForRoom(const QString& roomNumber, const QDateTime& checkIn,
                                    const QDateTime& checkOut, int adults, int children);
    void performBookingAction(const QString& bookingId, const QString& actionType);

signals:
    // Modified and optimized performance: notify dependent dashboard data only after checkout persistence completes.
    void bookingCompleted();
    void bookingChanged();
    void roomStatusBookingCancelled();

private slots:
    void onSearchChanged(const QString& text);
    void onFilterStatusChanged(int index);
    void onTableActionClicked();

private:
    void setupUI();
    void openReservationDialog(const QString& preselectedRoomNumber = QString(),
                               const QDateTime& initialCheckIn = QDateTime(), const QDateTime& initialCheckOut = QDateTime(),
                               int initialAdults = 1, int initialChildren = 0);

    HotelManager* m_manager;
    QString m_searchQuery;
    // Modified: Keep all reservation lifecycle filters available so completed records can expose invoice and audit actions.
    int m_statusFilterIndex; // 0: All, 1: Upcoming, 2: Active, 3: Completed, 4: Cancelled

    QLineEdit* m_searchEdit;
    QComboBox* m_statusCombo;
    QTableWidget* m_tableWidget;
};
