#pragma once

#include <QMainWindow>
#include <QTableWidget>
#include <QTabWidget>
#include "HotelManager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    // Receives the controller pointer for the core HotelManager system
    explicit MainWindow(HotelManager* mgr, QWidget *parent = nullptr);
    ~MainWindow();

    // Methods to update and redraw the UI grids for all data entities across tabs
    void updateRoomGrid();

    // Added: Refresh the booking tables based on their operational lifecycle state
    void updateBookingGrids();

    // Added: Refresh customer directories and historical CRM log layouts
    void updateCustomerGrid();

    // Added: Refresh the Dashboard metrics (Today's Arrivals / Departures statistics)
    void updateDashboard();

    void refreshAllViews();

    // Added: Helper methods for View-side processing before passing to Core
public:
    // Computes stay duration using Qt's QDate utility, acting as a bridge for MVC
    int getDurationInNights(const std::string& checkInStr, const std::string& checkOutStr) const;

private slots:
    // Slot to handle booking button click events
    void on_btnBook_clicked();

    // Slot to handle checkout button click events (Triggers Invoice generation & shifts state to Completed)
    void on_btnCheckout_clicked();

    // Added: Slot to handle soft-cancellation request for upcoming bookings
    void on_btnCancelBooking_clicked();

    // Added: Slot to capture row-selection shifts on the customer table to render their specific booking logs (CRM)
    void on_customerTable_itemSelectionChanged();

    // Added: Slot to sync updates automatically whenever the user switches between tab views
    void on_tabWidget_currentChanged(int index);

private:
    Ui::MainWindow *ui;

    // Standard mapping
    HotelManager* controller;
    QTableWidget* roomTable;
    QTabWidget* tabWidget;
};