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

private slots:
    void applyFilters();
    void setFilterType(QString type);

private:
    void setupUI();
    // Khong can createRoomStatusCard vi da dung RoomCard
    // QWidget* createRoomStatusCard(const std::shared_ptr<Room>& room);

    Ui::RoomStatusPageWidget *ui;
    HotelManager* m_manager;
    QList<RoomCard*> m_roomCards;
    bool m_isCheckAvailMode = false;

};
