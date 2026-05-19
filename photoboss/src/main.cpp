#include "ui/MainWindow.h"
#include "ui/ThumbnailManager.h"
#include "ui/PreviewPane.h"
#include "ui/DeletionService.h"
#include "ui/TrashDeletionStrategy.h"
#include "pipeline/PipelineController.h"

#include <QtWidgets/QApplication>
#include <QFile>
#include <exiv2/error.hpp>

int main(int argc, char *argv[])
{
    Exiv2::LogMsg::setLevel(Exiv2::LogMsg::mute);  // suppress EXIF parser warnings to stderr

    QApplication app(argc, argv);

    QFile f(":/styles/dark.qss");
    if (f.open(QFile::ReadOnly | QFile::Text)) {
        app.setStyleSheet(f.readAll());
    }

    auto controller = std::make_unique<photoboss::PipelineController>();
    auto previewPane = std::make_unique<photoboss::PreviewPane>();
    auto thumbnailManager = std::make_unique<photoboss::ThumbnailManager>(previewPane.get());
    auto deletionService = std::make_unique<photoboss::DeletionService>(
        thumbnailManager.get(),
        std::make_unique<photoboss::TrashDeletionStrategy>(),
        nullptr);

    photoboss::MainWindow window(
        std::move(controller),
        std::move(thumbnailManager),
        std::move(previewPane),
        std::move(deletionService));

    window.show();
    return app.exec();
}
