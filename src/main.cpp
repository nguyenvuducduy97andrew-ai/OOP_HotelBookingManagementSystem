#include <QApplication>
#include <QCoreApplication>
#include <QFile>
#include <QMessageBox>
#include <QDir>
#include <QStandardPaths>
#include <QString>
#include "mainwindow.h"
#include "LoginWindow.h"
#include "HotelManager.h"
#include "DataManager.h"

namespace {
QString findLegacyProjectDatabasePath()
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

    return QDir(searchDir.path()).filePath("data/hotel_data.db");
}

QString resolveDatabasePath()
{
    const QString dataDirPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (dataDirPath.isEmpty() || !QDir().mkpath(dataDirPath)) {
        return {};
    }

    const QString managedDatabasePath = QDir(dataDirPath).filePath("hotel_data.db");
    const QString legacyDatabasePath = findLegacyProjectDatabasePath();
    if (!QFile::exists(managedDatabasePath) && !legacyDatabasePath.isEmpty()
        && QFile::exists(legacyDatabasePath) && legacyDatabasePath != managedDatabasePath) {
        // Modified: Preserve a developer-run database once while moving runtime storage away from the working directory.
        if (!QFile::copy(legacyDatabasePath, managedDatabasePath)) {
            qWarning() << "Could not migrate legacy database to application data:" << legacyDatabasePath;
        }
    }

    return managedDatabasePath;
}
}

int main(int argc, char *argv[]) {
    // Initialize the Qt application.
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("VNUHCM-US");
    QCoreApplication::setApplicationName("HotelBookingManagement");
    // Create the HotelManager that owns the hotel's in-memory data.
    HotelManager hotelManager;

    const QString databasePath = resolveDatabasePath();

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
