#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>

namespace Ui {
class MainWindow;
}

class HotelManager;
class DashboardWidget;
class RoomPageWidget;
class ReservationsPageWidget;
class RoomStatusPageWidget;

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
    RoomStatusPageWidget* m_roomStatusPage;
    void updateButtonStyle(QPushButton* activeBtn);
};

#endif // MAINWINDOW_H