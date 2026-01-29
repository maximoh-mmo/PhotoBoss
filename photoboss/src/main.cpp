#include "ui/mainwindow.h"
#include "hashing/HashRegistry.h"
#include <QtWidgets/QApplication>
namespace photoboss
{
    int main(int argc, char* argv[]) {

        // Initialize the application
        HashRegistry::initializeBuiltIns();
        QApplication app(argc, argv);
        photoboss::MainWindow window;
        window.show();
        return app.exec();
    }
}