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