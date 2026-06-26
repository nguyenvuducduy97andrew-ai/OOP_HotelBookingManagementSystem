#include <QApplication>
#include <QMessageBox>
#include "MainWindow.h"
#include "HotelManager.h"
#include "DataManager.h"

int main(int argc, char *argv[]) {
// Khởi tạo ứng dụng Qt
    QApplication app(argc, argv);
// Tạo một đối tượng HotelManager để quản lý dữ liệu khách sạn
    HotelManager hotelManager;
//Khởi tạo và tải dữ liệu từ cơ sở dữ liệu SQLite
    if (!DataManager::getInstance().loadAll(hotelManager, "hotel_data.db")) {
        QMessageBox::critical(nullptr, "Database Error", "Failed to load or initialize the database file. The application will close.");
        return -1;
    }
// Tạo cửa sổ chính của ứng dụng và hiển thị nó
    MainWindow mainWindow(&hotelManager);
    mainWindow.show();
// Giữ ứng dụng chạy cho đến khi người dùng đóng cửa sổ chính
    int result = app.exec();
// Khi ứng dụng kết thúc, lưu dữ liệu trở lại cơ sở dữ liệu SQLite
    if (!DataManager::getInstance().saveAll(hotelManager, "hotel_data.db")) {
        QMessageBox::warning(nullptr, "Save Warning", "Database warning: Data changes during this session could not be saved.");
    }

    return result;
}