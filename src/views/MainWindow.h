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
    // Nhận con trỏ điều khiển bộ lõi HotelManager
    explicit MainWindow(HotelManager* mgr, QWidget *parent = nullptr);
    ~MainWindow();

    // Hàm cập nhật và vẽ lại lưới hiển thị danh sách phòng lên UI
    void updateRoomGrid();

private slots:
    // Slot xử lý sự kiện bấm nút đặt phòng
    void on_btnBook_clicked();

    // Slot xử lý sự kiện trả phòng
    void on_btnCheckout_clicked();

private:
    Ui::MainWindow *ui;

    // Ánh xạ chuẩn
    HotelManager* controller;
    QTableWidget* roomTable;
    QTabWidget* tabWidget;
};