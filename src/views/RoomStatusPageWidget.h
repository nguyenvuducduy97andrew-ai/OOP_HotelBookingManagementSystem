#pragma once
#include <QWidget>
#include <QList>
#include "HotelManager.h"
#include "roomcard.h"

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
    void bookingRequested(const QString& roomNumber);

private slots:
    void applyFilters();
    void setFilterType(QString type);

private:
    void setupUI();
    // RoomCard already provides the room status card UI.
    // QWidget* createRoomStatusCard(const std::shared_ptr<Room>& room);

    Ui::RoomStatusPageWidget *ui;
    HotelManager* m_manager;
    QList<RoomCard*> m_roomCards;
    bool m_isCheckAvailMode = false;

};
