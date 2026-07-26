#include <QApplication>
#include <QMessageBox>
#include <QDir>
// Force rebuild comment
#include "mainwindow.h"
#include "HotelManager.h"
#include "DataManager.h"

namespace {
QString resolveDatabasePath()
{
    QDir searchDir(QDir::currentPath());

    while (true) {
        if (searchDir.exists("CMakeLists.txt") && searchDir.exists("src")) {
            break;
        }

        if (!searchDir.cdUp()) {
            break;
        }
    }

    const QString projectRoot = searchDir.path();
    const QString dataDirPath = QDir(projectRoot).filePath("data");
    QDir().mkpath(dataDirPath);

    return QDir(dataDirPath).filePath("hotel_data.db");
}
}

int main(int argc, char *argv[]) {
    // Khởi tạo ứng dụng Qt
    QApplication app(argc, argv);
    // Tạo một đối tượng HotelManager để quản lý dữ liệu khách sạn
    HotelManager hotelManager;

    const QString databasePath = resolveDatabasePath();

    // Load initial hotel database records into the core engine state at startup
    if (!DataManager::getInstance().loadAll(hotelManager, databasePath.toStdString())) {
        QMessageBox::critical(nullptr, "Database Error", "Failed to load or initialize the database file. The application will close.");
        return -1;
    }

    // Tự động tạo 100 phòng demo nếu chưa có dữ liệu phòng nào
    if (hotelManager.getRooms().empty()) {
        std::string err;
        for (int i = 0; i < 100; ++i) {
            std::string roomNum = std::to_string(101 + i);
            RoomType type = RoomType::Standard;
            double price = 500000;
            
            if (i % 3 == 0) {
                type = RoomType::Suite;
                price = 2000000;
            } else if (i % 2 == 0) {
                type = RoomType::Deluxe;
                price = 1000000;
            }
            
            hotelManager.registerRoom(type, roomNum, price, err);
            
            if (i == 2) {
                auto room = hotelManager.findRoomByNumber(roomNum);
                if (room) room->setIsAvailable(false); // Maintenance
            } else if (i % 2 == 0) {
                // Tạo khách hàng và booking giả lập để phòng thành OCC
                std::string custId = "CCCD_" + std::to_string(i);
                hotelManager.registerCustomer(custId, "Nguyen Van Kai", "0901234567", err);
                
                std::string dateIn = QDate::currentDate().toString("yyyy-MM-dd").toStdString();
                std::string dateOut = QDate::currentDate().addDays(1).toString("yyyy-MM-dd").toStdString();
                hotelManager.createBooking(custId, roomNum, dateIn, dateOut, err);
            }
        }
        DataManager::getInstance().saveAll(hotelManager, databasePath.toStdString());
    }

    // Inject the managed core logic pointer into the UI view layer and display window
    MainWindow mainWindow(&hotelManager);
    mainWindow.show();

    // Block and spin execution inside Qt's event loop processing loop
    int result = app.exec();

    // Modified: Invokes non-const saveAll to write out data changes before application process terminates
    if (!DataManager::getInstance().saveAll(hotelManager, databasePath.toStdString())) {
        QMessageBox::warning(nullptr, "Save Warning", "Database warning: Data changes during this session could not be saved.");
    }

    return result;
}