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

    // Method to update and redraw the room listing grid on the UI
    void updateRoomGrid();

    // Added: Helper methods for View-side processing before passing to Core
public:
    // Computes stay duration using Qt's QDate utility, acting as a bridge for MVC
    int getDurationInNights(const std::string& checkInStr, const std::string& checkOutStr) const;

private slots:
    // Slot to handle booking button click events
    void on_btnBook_clicked();

    // Slot to handle checkout button click events
    void on_btnCheckout_clicked();

private:
    Ui::MainWindow *ui;

    // Standard mapping
    HotelManager* controller;
    QTableWidget* roomTable;
    QTabWidget* tabWidget;
};