#include <QApplication>
#include <QMessageBox>
#include <QDir>
#include <QString>
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
    // Initialize the Qt application.
    QApplication app(argc, argv);
    // Create the HotelManager that owns the hotel's in-memory data.
    HotelManager hotelManager;

    const QString databasePath = resolveDatabasePath();

    // Load initial hotel database records into the core engine state at startup
    if (!DataManager::getInstance().loadAll(hotelManager, databasePath.toStdString())) {
        QMessageBox::critical(nullptr, "Database Error", "Failed to load or initialize the database file. The application will close.");
        return -1;
    }

    // Automatically seed 100 demo rooms when no room data exists yet.
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
                hotelManager.scheduleRoomMaintenance(
                    roomNum,
                    QDate::currentDate().toString(Qt::ISODate).toStdString(),
                    QDate::currentDate().addDays(1).toString(Qt::ISODate).toStdString(),
                    "Demo maintenance", err);
            } else if (i % 2 == 0) {
                // Create demo customers and bookings so rooms become occupied.
                std::string custId = QString("%1").arg(100000000000ULL + static_cast<unsigned long long>(i), 12, 10, QChar('0')).toStdString();
                hotelManager.registerCustomer(custId, "Nguyen Van Kai", "+84901234567", err);
                
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
