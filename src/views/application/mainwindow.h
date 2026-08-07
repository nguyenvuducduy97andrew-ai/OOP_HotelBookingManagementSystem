#pragma once

#include <QMainWindow>
#include <QPushButton>

namespace Ui {
class MainWindow;
}

class HotelManager;
class DashboardWidget;
class RoomPageWidget;
class ReservationsPageWidget;
class CustomerPageWidget;
class RoomStatusPageWidget;
class StatisticsPageWidget;
class QStackedWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(HotelManager* manager, QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    HotelManager* m_manager;
    DashboardWidget* m_dashboardPage;
    RoomPageWidget* m_roomPage;
    ReservationsPageWidget* m_reservationsPage;
    CustomerPageWidget* m_customerPage;
    RoomStatusPageWidget* m_roomStatusPage;
    StatisticsPageWidget* m_statisticsPage;
    QWidget* m_reservationContainer = nullptr;
    QStackedWidget* m_reservationSubPages = nullptr;
    QPushButton* m_roomStatusTab = nullptr;
    QPushButton* m_reservationDetailsTab = nullptr;
    void updateButtonStyle(QPushButton* activeBtn);
    void showReservationSubPage(int index);
};
