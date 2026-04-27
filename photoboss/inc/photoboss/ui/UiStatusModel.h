#pragma once

#include <QObject>
#include <QMutex>
#include <QPixmap>
#include <QMap>
#include <QMultiMap>
#include <deque>

#include "types/DataTypes.h"
#include "types/GroupTypes.h"
#include "pipeline/PipelineController.h"

namespace photoboss {
class ImageThumbWidget;

/**
 * UiStatusModel – thread‑safe container for data that the UI polls.
 * Owned by MainWindow (no global singleton).
 */
class UiStatusModel : public QObject {
    Q_OBJECT
public:
    explicit UiStatusModel(QObject* parent = nullptr);

    // Update methods – called from signal slots (queued to the UI thread)
    void addPendingGroup(const ImageGroup& group);
    void updateGroup(const ImageGroup& group);
    void setThumbnail(const ThumbnailResult& result);
    void setPhaseProgress(PipelineController::Phase phase, int count, int total);
    void setStatusMessage(const QString& msg);
    void setPipelineState(PipelineController::PipelineState state);

    // Snapshot – copy of all data for the poller (read‑only)
    struct Snapshot {
        std::deque<ImageGroup> pendingGroups;
        QMap<quint64, ImageGroup> updatedGroups;          // keyed by group id
        QMap<QString, QPixmap> thumbnailCache;
        QMultiMap<QString, ImageThumbWidget*> thumbnailWaiters;
        QMap<PipelineController::Phase, std::pair<int,int>> phaseProgress;
        QString statusMessage;
        PipelineController::PipelineState pipelineState = PipelineController::PipelineState::Stopped;
        bool operator==(const Snapshot& other) const;
    };
    Snapshot snapshot() const;
    bool operator!=(const Snapshot& other) const;

private:
    mutable QMutex m_mutex;
    // internal storage
    std::deque<ImageGroup> m_pendingGroups;
    QMap<quint64, ImageGroup> m_updatedGroups;
    QMap<QString, QPixmap> m_thumbnailCache;
    QMultiMap<QString, ImageThumbWidget*> m_thumbnailWaiters;
    QMap<PipelineController::Phase, std::pair<int,int>> m_phaseProgress;
    QString m_statusMessage;
    PipelineController::PipelineState m_pipelineState = PipelineController::PipelineState::Stopped;
};

} // namespace photoboss
