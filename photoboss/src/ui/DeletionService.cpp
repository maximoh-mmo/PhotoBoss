#include "ui/DeletionService.h"
#include "ui/IDeletionStrategy.h"
#include "ui/ThumbnailManager.h"
#include "ui/GroupWidget.h"
#include "ui/DeleteConfirmDialog.h"

#include <QFile>
#include <QMessageBox>

namespace photoboss {

DeletionService::DeletionService(ThumbnailManager* thumbnailManager,
                                 std::unique_ptr<IDeletionStrategy> strategy,
                                 QWidget* parentWidget,
                                 QObject* parent)
    : QObject(parent)
    , m_thumbnailManager_(thumbnailManager)
    , m_strategy_(std::move(strategy))
    , m_parentWidget_(parentWidget)
{
}

int DeletionService::countSelected() const
{
    int count = 0;
    for (auto* widget : m_thumbnailManager_->groupWidgets()) {
        count += widget->countSelectedForDeletion();
    }
    return count;
}

QVector<ImageEntry> DeletionService::collectSelected() const
{
    QVector<ImageEntry> result;
    for (auto* widget : m_thumbnailManager_->groupWidgets()) {
        const auto& marked = widget->imagesMarkedForDeleteEntries();
        for (const auto& img : marked) {
            result.push_back(img);
        }
    }
    return result;
}

void DeletionService::executeDeletion()
{
    auto files = collectSelected();
    if (files.isEmpty())
        return;

    DeleteConfirmDialog dialog(files, m_parentWidget_);
    if (dialog.exec() != QDialog::Accepted)
        return;

    QStringList failedFiles;
    for (const auto& file : files) {
        if (!m_strategy_->deleteFile(file.path)) {
            failedFiles.append(file.path);
        }
    }

    if (failedFiles.isEmpty()) {
        QMessageBox::information(
            m_parentWidget_,
            "Delete Successful",
            QString("Successfully moved %1 file(s) to trash.").arg(files.size()));
    } else {
        QMessageBox::warning(
            m_parentWidget_,
            "Partial Failure",
            QString("Failed to move %1 file(s) to trash:\n%2")
                .arg(failedFiles.size())
                .arg(failedFiles.join("\n")));
    }

    emit deletionCompleted();
}

} // namespace photoboss
