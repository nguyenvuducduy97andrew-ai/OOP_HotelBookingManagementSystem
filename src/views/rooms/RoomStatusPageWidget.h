#pragma once
#include <QWidget>
#include <QList>
#include <QDateTime>
#include <QMap>
#include <QTimer>
#include <unordered_set>
#include "HotelManager.h"
#include "roomcard.h"

class QLabel;
class QPushButton;
class QResizeEvent;
class QComboBox;
class QGridLayout;

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
    void clearFilters();
    void rebuildFloorSections();
    int floorColumnCount() const;
    void clearFloorSections();
    QString roomStateSignature() const;
    // RoomCard already provides the room status card UI.
    // QWidget* createRoomStatusCard(const std::shared_ptr<Room>& room);

    Ui::RoomStatusPageWidget *ui;
    HotelManager* m_manager;
    QList<RoomCard*> m_roomCards;
    QList<QWidget*> m_floorSections;
    QMap<int, QList<RoomCard*>> m_cardsByFloor;
    QMap<int, QWidget*> m_floorSectionByNumber;
    QMap<int, QGridLayout*> m_floorLayoutByNumber;
    int m_lastFloorColumnCount = 0;
    QString m_lastRoomStateSignature;
    bool m_isCheckAvailMode = false;
    QTimer* m_statusRefreshTimer = nullptr;
    QTimer* m_searchDebounceTimer = nullptr;
    bool m_reflowPending = false;
    QDateTime m_selectedCheckIn;
    QDateTime m_selectedCheckOut;
    QDateTime m_cachedAvailabilityCheckIn;
    QDateTime m_cachedAvailabilityCheckOut;
    std::unordered_set<std::string> m_cachedAvailableRoomNumbers;
    bool m_availabilityCacheValid = false;
    QPushButton* m_scheduleButton = nullptr;
    QPushButton* m_checkInScheduleField = nullptr;
    QPushButton* m_checkOutScheduleField = nullptr;
    QComboBox* m_roomTypeCombo = nullptr;

protected:
    void resizeEvent(QResizeEvent* event) override;

};
