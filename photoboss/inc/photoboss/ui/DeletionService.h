#pragma once

#include <QObject>
#include <QVector>
#include <memory>

#include "types/GroupTypes.h"
#include "ui/IDeletionStrategy.h"

namespace photoboss {
class ThumbnailManager;

class DeletionService : public QObject {
    Q_OBJECT

public:
    DeletionService(ThumbnailManager* thumbnailManager,
                    std::unique_ptr<IDeletionStrategy> strategy,
                    QWidget* parentWidget,
                    QObject* parent = nullptr);

    int countSelected() const;
    void executeDeletion();
    void setDialogParent(QWidget* widget) { m_parentWidget_ = widget; }

signals:
    void deletionCompleted();

private:
    QVector<ImageEntry> collectSelected() const;

    ThumbnailManager* m_thumbnailManager_;
    std::unique_ptr<IDeletionStrategy> m_strategy_;
    QWidget* m_parentWidget_;
};

} // namespace photoboss
