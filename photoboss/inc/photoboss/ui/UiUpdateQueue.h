#pragma once

#include <QObject>
#include <QMutex>
#include <QPixmap>
#include <QMap>
#include <QMultiMap>
#include <deque>
#include <vector>

#include "types/DataTypes.h"
#include "types/GroupTypes.h"
#include "pipeline/Pipeline.h"
#include "ui/IUiUpdateSink.h"
#include "ui/UiSnapshot.h"

namespace photoboss {
class ImageThumbWidget;
/**
 * UiUpdateQueue – thread‑safe container for UI‑relevant data.
 *
 * Pipeline workers update the queue via the public mutator methods.
 * Each mutator marks the queue as dirty and schedules a single
 * queued emission of a snapshot (coalesced view of all data).
 *
 * Consumers (e.g. MainWindow) connect to the `snapshotReady`
 * signal and apply the data to the UI in one batch.
 */
class UiUpdateQueue : public QObject, public IUiUpdateSink {
	Q_OBJECT
public:
    explicit UiUpdateQueue(QObject* parent = nullptr);

    // Reset all state for a new scan
    void reset();

    // Update methods – called from any thread (queued to UI thread)
    void addPendingGroup(const ImageGroup& group) override;
    void updateGroup(const ImageGroup& group) override;
    void setThumbnail(const ThumbnailResult& result) override;
    void incrementPhaseProgress(Pipeline::Phase phase, int increment) override;
    void setFileTotal(int total) override;
    void setStatusMessage(const QString& msg) override;
    void setPipelineState(Pipeline::PipelineState state) override;

    // Commit processed groups – removes them from the pending queue
    void commitProcessed(int count);

    using Snapshot = UiSnapshot;

    UiSnapshot snapshot() const;

signals:
    void snapshotReady(const UiSnapshot& snap);

private slots:
    void maybeEmitSnapshot();

private:
    void scheduleSnapshotEmit();

    mutable QRecursiveMutex m_mutex;
    bool m_dirty = false;          // true when any mutator changed state
    bool m_emitPending = false;    // true when a queued emit is already scheduled

    // Track total files from Find phase for use in subsequent phases
    int m_totalFiles = 0;

    // internal storage – same layout as the original UiStatusModel
    std::deque<ImageGroup> m_pendingGroups;
    QMap<quint64, ImageGroup> m_updatedGroups;
    QMap<QString, QPixmap> m_thumbnailCache;
    QMultiMap<QString, ImageThumbWidget*> m_thumbnailWaiters;
    QMap<Pipeline::Phase, std::pair<int,int>> m_phaseProgress;
    QString m_statusMessage;
	Pipeline::PipelineState m_pipelineState = Pipeline::PipelineState::Stopped;
};

} // namespace photoboss
