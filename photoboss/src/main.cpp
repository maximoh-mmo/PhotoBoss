#include "ui/mainwindow.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[]) {

    // Initialize the application
    QApplication app(argc, argv);

    QFile f(":/styles/dark.qss");
    if (f.open(QFile::ReadOnly | QFile::Text)) {
        app.setStyleSheet(f.readAll());
    }

    photoboss::MainWindow window;
    window.show();
    return app.exec();
}
