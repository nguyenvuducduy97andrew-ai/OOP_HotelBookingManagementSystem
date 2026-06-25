#include <QApplication>
#include <QMessageBox>
#include "MainWindow.h"
#include "HotelManager.h"
#include "DataManager.h"

int main(int argc, char *argv[]) {

    QApplication app(argc, argv);

    HotelManager hotelManager;

    if (!DataManager::getInstance().loadAll(hotelManager, "hotel_data.db")) {
        QMessageBox::critical(nullptr, "Database Error", "Failed to load or initialize the database file. The application will close.");
        return -1;
    }

    MainWindow mainWindow(&hotelManager);
    mainWindow.show();

    int result = app.exec();

    if (!DataManager::getInstance().saveAll(hotelManager, "hotel_data.db")) {
        QMessageBox::warning(nullptr, "Save Warning", "Database warning: Data changes during this session could not be saved.");
    }

    return result;
}