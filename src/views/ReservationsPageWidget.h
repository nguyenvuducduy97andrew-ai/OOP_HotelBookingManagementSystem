#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include "HotelManager.h"

class ReservationsPageWidget : public QWidget {
    Q_OBJECT

public:
    explicit ReservationsPageWidget(HotelManager* manager, QWidget *parent = nullptr);

    void refreshData();

signals:
    // Modified and optimized performance: notify dependent dashboard data only after checkout persistence completes.
    void bookingCompleted();

private slots:
    void onSearchChanged(const QString& text);
    void onFilterStatusChanged(int index);
    void onAddBookingClicked();
    void onTableActionClicked();

private:
    void setupUI();

    HotelManager* m_manager;
    QString m_searchQuery;
    int m_statusFilterIndex; // 0: All, 1: Upcoming, 2: Active, 3: Cancelled

    QLineEdit* m_searchEdit;
    QComboBox* m_statusCombo;
    QPushButton* m_addBookingBtn;
    QTableWidget* m_tableWidget;
};
