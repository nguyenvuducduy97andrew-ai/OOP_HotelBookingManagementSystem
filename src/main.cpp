#include <QApplication>

#include "DataManager.h"
#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    DataManager::getInstance().loadAll();

    MainWindow window;
    window.show();

    return app.exec();
}
