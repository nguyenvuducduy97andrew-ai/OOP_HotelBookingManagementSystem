#include <QApplication>
#include <QCoreApplication>
#include <QMessageBox>
#include <QDir>
#include <QString>
#include "mainwindow.h"
#include "LoginWindow.h"
#include "HotelManager.h"
#include "DataManager.h"

namespace {
QString resolveProjectDatabasePath()
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

    if (!searchDir.exists("CMakeLists.txt") || !searchDir.exists("src")) {
        return {};
    }

    const QString dataDirPath = QDir(searchDir.path()).filePath("data");
    if (!QDir().mkpath(dataDirPath)) {
        return {};
    }
    // Modified: Use only the project data file; SQLite creates it empty here when no database exists.
    return QDir(dataDirPath).filePath("hotel_data.db");
}
}

int main(int argc, char *argv[]) {
    // Initialize the Qt application.
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("VNUHCM-US");
    QCoreApplication::setApplicationName("HotelBookingManagement");
    // Create the HotelManager that owns the hotel's in-memory data.
    HotelManager hotelManager;

    const QString databasePath = resolveProjectDatabasePath();
    // Modified: Refuse to start outside the project tree instead of opening an unspecified SQLite path.
    if (databasePath.isEmpty()) {
        QMessageBox::critical(nullptr, "Database Error", "The project data folder could not be located. Run the application from inside the project tree.");
        return -1;
    }

    // Load initial hotel database records into the core engine state at startup
    if (!DataManager::getInstance().loadAll(hotelManager, databasePath.toStdString())) {
        QMessageBox::critical(nullptr, "Database Error", "Failed to load or initialize the database file. The application will close.");
        return -1;
    }

    // Modified: Leave a newly initialized database empty; staff create all operational data through the application.

    // Modified: Authenticate the staff member before constructing the operational window with the shared HotelManager.
    LoginWindow loginWindow;
    if (loginWindow.exec() != QDialog::Accepted) {
        return 0;
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
