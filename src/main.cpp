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
QString applicationScrollBarStyleSheet()
{
    return QStringLiteral(R"(
        QScrollBar:vertical {
            background: transparent;
            width: 12px;
            margin: 5px 3px 5px 0;
        }
        QScrollBar::handle:vertical {
            background: #C7D3E3;
            border: 2px solid transparent;
            border-radius: 5px;
            min-height: 34px;
        }
        QScrollBar::handle:vertical:hover {
            background: #94A9C2;
        }
        QScrollBar::handle:vertical:pressed {
            background: #6E86A3;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: transparent;
        }
        QScrollBar:horizontal {
            background: transparent;
            height: 12px;
            margin: 0 5px 3px 5px;
        }
        QScrollBar::handle:horizontal {
            background: #C7D3E3;
            border: 2px solid transparent;
            border-radius: 5px;
            min-width: 34px;
        }
        QScrollBar::handle:horizontal:hover {
            background: #94A9C2;
        }
        QScrollBar::handle:horizontal:pressed {
            background: #6E86A3;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
        }
        QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal,
        QAbstractScrollArea::corner {
            background: transparent;
        }
    )");
}

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
    // Modified: apply the Dashboard-inspired rounded scrollbar treatment globally to every scrollable application control.
    app.setStyleSheet(applicationScrollBarStyleSheet());
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
