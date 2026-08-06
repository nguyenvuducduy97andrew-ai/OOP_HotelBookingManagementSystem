#pragma once
#include <QWidget>
#include <QList>
#include <QDateTime>
#include <QTimer>
#include "HotelManager.h"
#include "roomcard.h"

class QLabel;
class QPushButton;

namespace Ui {
class RoomStatusPageWidget;
}

class RoomStatusPageWidget : public QWidget {
    Q_OBJECT

public:
    explicit RoomStatusPageWidget(HotelManager* manager, QWidget *parent = nullptr);

    void refreshData();

    ~RoomStatusPageWidget();

signals:
    // Modified and optimized performance: hand off booking navigation to MainWindow instead of creating bookings from the room-status page.
    void bookingRequested(const QString& roomNumber, const QDateTime& checkIn, const QDateTime& checkOut, int adults, int children);

private slots:
    void applyFilters();
    void setFilterType(QString type);

private:
    void openSchedulePicker(bool startInCheckOutMode);
    void updateScheduleFields();
    void setupUI();
    void setAvailabilityMode(bool enabled);
    // RoomCard already provides the room status card UI.
    // QWidget* createRoomStatusCard(const std::shared_ptr<Room>& room);

    Ui::RoomStatusPageWidget *ui;
    HotelManager* m_manager;
    QList<RoomCard*> m_roomCards;
    bool m_isCheckAvailMode = false;
    QTimer* m_statusRefreshTimer = nullptr;
    QDateTime m_selectedCheckIn;
    QDateTime m_selectedCheckOut;
    QPushButton* m_scheduleButton = nullptr;
    QPushButton* m_checkInScheduleField = nullptr;
    QPushButton* m_checkOutScheduleField = nullptr;

};
