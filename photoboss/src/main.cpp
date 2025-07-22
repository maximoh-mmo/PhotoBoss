#include "mainwindow.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[]) {

    // Initialize the application
    QApplication app(argc, argv);
    photoboss::MainWindow window;
    window.show();
    return app.exec();
}
