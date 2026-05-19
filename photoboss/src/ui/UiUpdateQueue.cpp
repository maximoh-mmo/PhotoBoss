#include "ui/UiUpdateQueue.h"
#include "util/AppSettings.h"
#include <QDebug>

namespace photoboss {

UiUpdateQueue::UiUpdateQueue(QObject* parent)
    : QObject(parent)
{
    m_throttleTimer_.setInterval(settings::UiPollIntervalMs);
    m_throttleTimer_.setSingleShot(false);
    connect(&m_throttleTimer_, &QTimer::timeout,
            this, &UiUpdateQueue::maybeEmitSnapshot);
}

void UiUpdateQueue::reset()
{
    QMutexLocker lock(&m_mutex);
    m_pendingGroups.clear();
    m_updatedGroups.clear();
    m_modifiedGroupIds_.clear();
    m_thumbnailCache.clear();
    m_thumbnailWaiters.clear();
    m_phaseProgress.clear();
    m_statusMessage.clear();
    m_pipelineState = Pipeline::PipelineState::Stopped;
    m_totalFiles = 0;
    m_dirty = true;
	m_statusMessage = "";
    lock.unlock();
    scheduleSnapshotEmit();
}

// Helper: ensure the throttle timer is running (called from any thread)
void UiUpdateQueue::scheduleSnapshotEmit()
{
    QMetaObject::invokeMethod(this, [this]() {
        if (!m_throttleTimer_.isActive())
            m_throttleTimer_.start();
    }, Qt::QueuedConnection);
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
    m_modifiedGroupIds_.insert(group.id);
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

void UiUpdateQueue::incrementPhaseProgress(Pipeline::Phase phase, int increment)
{
    QMutexLocker lock(&m_mutex);
    auto& entry = m_phaseProgress[phase];
    entry.first += increment;
    // If total hasn't been set yet and we have a file total, use it
    if (entry.second == 0 && m_totalFiles > 0) {
        entry.second = m_totalFiles;
    }
    m_dirty = true;
    lock.unlock();
    scheduleSnapshotEmit();
}

void UiUpdateQueue::setFileTotal(int total)
{
    QMutexLocker lock(&m_mutex);
    m_totalFiles = total;
    // Propagate this total to all phase progress entries (both existing and future)
    for (auto& entry : m_phaseProgress) {
        entry.second = total;
    }
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

UiSnapshot UiUpdateQueue::snapshot()
{
    QMutexLocker lock(&m_mutex);
    UiSnapshot snap;
    snap.pendingGroups = m_pendingGroups;
    for (quint64 id : m_modifiedGroupIds_) {
        auto it = m_updatedGroups.find(id);
        if (it != m_updatedGroups.end())
            snap.updatedGroups.insert(id, it.value());
    }
    m_modifiedGroupIds_.clear();
    snap.thumbnailCache = m_thumbnailCache;
    snap.thumbnailWaiters = m_thumbnailWaiters;
    snap.phaseProgress = m_phaseProgress;
    snap.statusMessage = m_statusMessage;
    snap.pipelineState = m_pipelineState;
    return snap;
}


bool UiSnapshot::operator==(const UiSnapshot& other) const
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
    {
        QMutexLocker lock(&m_mutex);
        if (!m_dirty) {
            m_throttleTimer_.stop();
            return;
        }
        m_dirty = false;
    }
    emit snapshotReady(this->snapshot());
}

} // namespace photoboss
