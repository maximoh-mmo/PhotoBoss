#include "ui/UiUpdateQueue.h"
#include <QDebug>

namespace photoboss {

UiUpdateQueue::UiUpdateQueue(QObject* parent)
    : QObject(parent)
{
}

void UiUpdateQueue::reset()
{
    QMutexLocker lock(&m_mutex);
    m_pendingGroups.clear();
    m_updatedGroups.clear();
    m_thumbnailCache.clear();
    m_thumbnailWaiters.clear();
    m_phaseProgress.clear();
    m_statusMessage.clear();
    m_pipelineState = Pipeline::PipelineState::Stopped;
    m_totalFiles = 0;
    m_dirty = true;
    lock.unlock();
    scheduleSnapshotEmit();
}

// Helper: schedule a single queued emission if none is pending
void UiUpdateQueue::scheduleSnapshotEmit()
{
    if (!m_emitPending) {
        m_emitPending = true;
        // Use QueuedConnection so the call is executed on the UI thread
        QMetaObject::invokeMethod(this,
            &UiUpdateQueue::maybeEmitSnapshot,
            Qt::QueuedConnection);
    }
}

void UiUpdateQueue::addPendingGroup(const ImageGroup& group)
{
    QMutexLocker lock(&m_mutex);
    m_pendingGroups.push_back(group);
    m_dirty = true;
    lock.unlock();
    scheduleSnapshotEmit();
}

void UiUpdateQueue::updateGroup(const ImageGroup& group)
{
    QMutexLocker lock(&m_mutex);
    m_updatedGroups[group.id] = group;
    m_dirty = true;
    lock.unlock();
    scheduleSnapshotEmit();
}

void UiUpdateQueue::commitProcessed(int count)
{
    QMutexLocker lock(&m_mutex);
    for (int i = 0; i < count && !m_pendingGroups.empty(); ++i) {
        m_pendingGroups.pop_front();
    }
}

void UiUpdateQueue::setThumbnail(const ThumbnailResult& result)
{
    QMutexLocker lock(&m_mutex);
    m_thumbnailCache[result.path] = QPixmap::fromImage(result.image);
    m_dirty = true;
    lock.unlock();
    scheduleSnapshotEmit();
}

void UiUpdateQueue::setPhaseProgress(Pipeline::Phase phase, int count, int total)
{
    QMutexLocker lock(&m_mutex);

    // For Analyze and Group phases, use the Find phase total as expected total
    // if no explicit total is provided
    int displayTotal = total;
    if (m_totalFiles == 0 && count > 0) {
        if (phase == Pipeline::Phase::Analyze && m_totalFiles > 0) {
            displayTotal = m_totalFiles;
        } else if (phase == Pipeline::Phase::Group && m_totalFiles > 0) {
            displayTotal = m_totalFiles;
        }
    }
    m_phaseProgress[phase] = { count, displayTotal };
    m_dirty = true;
    lock.unlock();
    scheduleSnapshotEmit();
}

void UiUpdateQueue::setFileTotal(int total)
{
    QMutexLocker lock(&m_mutex);
    m_totalFiles = total;
    m_dirty = true;
    lock.unlock();
    scheduleSnapshotEmit();
}

void UiUpdateQueue::setStatusMessage(const QString& msg)
{
    QMutexLocker lock(&m_mutex);
    m_statusMessage = msg;
    m_dirty = true;
    lock.unlock();
    scheduleSnapshotEmit();
}

void UiUpdateQueue::setPipelineState(Pipeline::PipelineState state)
{
    QMutexLocker lock(&m_mutex);
    m_pipelineState = state;
    m_dirty = true;
    lock.unlock();
    scheduleSnapshotEmit();
}

UiUpdateQueue::Snapshot UiUpdateQueue::snapshot() const
{
    QMutexLocker lock(&m_mutex);
    Snapshot snap;
    snap.pendingGroups = m_pendingGroups;
    snap.updatedGroups = m_updatedGroups;
    snap.thumbnailCache = m_thumbnailCache;
    snap.thumbnailWaiters = m_thumbnailWaiters;
    snap.phaseProgress = m_phaseProgress;
    snap.statusMessage = m_statusMessage;
    snap.pipelineState = m_pipelineState;
    return snap;
}

bool UiUpdateQueue::operator!=(const Snapshot& other) const
{
    return !(this->snapshot() == other);
}

bool UiUpdateQueue::Snapshot::operator==(const Snapshot& other) const
{
    // Equality is based on data that influences UI rendering.
    // We deliberately exclude containers that contain types without ==
    // (ImageGroup, QPixmap) and also exclude pendingGroups and thumbnailWaiters,
    // which are transient.
    return phaseProgress == other.phaseProgress &&
           statusMessage == other.statusMessage &&
           pipelineState == other.pipelineState;
}

void UiUpdateQueue::maybeEmitSnapshot()
{
    Snapshot snap;
    {
        QMutexLocker lock(&m_mutex);
        if (!m_dirty) {
            m_emitPending = false;
            return; // nothing changed since we were scheduled
        }
        snap = this->snapshot(); // copy under lock
        m_dirty = false;
        m_emitPending = false;
    }
    // Emit outside the lock to avoid re‑entrancy issues
    emit snapshotReady(snap);
}

} // namespace photoboss
