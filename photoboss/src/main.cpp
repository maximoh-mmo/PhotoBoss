#include "ui/mainwindow.h"
#include <QtWidgets/QApplication>
#include "hashing/HashRegistry.h"

int main(int argc, char *argv[]) {

    // Initialize the application
    photoboss::HashRegistry::initializeBuiltIns();
    QApplication app(argc, argv);
    photoboss::MainWindow window;
    window.show();
    return app.exec();
}
