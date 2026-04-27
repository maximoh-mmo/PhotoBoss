#include "ui/UiStatusModel.h"

namespace photoboss {

    UiStatusModel::UiStatusModel(QObject* parent)
        : QObject(parent)
    {
    }

    void UiStatusModel::addPendingGroup(const ImageGroup& group)
    {
        QMutexLocker lock(&m_mutex);
        m_pendingGroups.push_back(group);
    }

    void UiStatusModel::updateGroup(const ImageGroup& group)
    {
        QMutexLocker lock(&m_mutex);
        m_updatedGroups[group.id] = group;
    }

    void UiStatusModel::setThumbnail(const ThumbnailResult& result)
    {
        QMutexLocker lock(&m_mutex);
        m_thumbnailCache[result.path] = QPixmap::fromImage(result.image);
        // Note: waiters are handled by UI after snapshot; we only store the cache here.
    }

    void UiStatusModel::setPhaseProgress(PipelineController::Phase phase, int count, int total)
    {
        QMutexLocker lock(&m_mutex);
        m_phaseProgress[phase] = { count, total };
    }

    void UiStatusModel::setStatusMessage(const QString& msg)
    {
        QMutexLocker lock(&m_mutex);
        m_statusMessage = msg;
    }

    void UiStatusModel::setPipelineState(PipelineController::PipelineState state)
    {
        QMutexLocker lock(&m_mutex);
        m_pipelineState = state;
    }

    UiStatusModel::Snapshot UiStatusModel::snapshot() const
    {
        QMutexLocker lock(&m_mutex);
        Snapshot snap;
        snap.pendingGroups = m_pendingGroups;               // copy
        snap.updatedGroups = m_updatedGroups;
        snap.thumbnailCache = m_thumbnailCache;
        snap.thumbnailWaiters = m_thumbnailWaiters;
        snap.phaseProgress = m_phaseProgress;
        snap.statusMessage = m_statusMessage;
        snap.pipelineState = m_pipelineState;
        return snap;
    }

    bool UiStatusModel::operator!=(const Snapshot& other) const
    {
        return !(this->snapshot() == other);
    }

    // Equality operator for UiStatusModel::Snapshot (used for early‑out)

    // Snapshot equality – compares all fields
bool UiStatusModel::Snapshot::operator==(const Snapshot& other) const
{
    // Equality is based on data that influences UI rendering.
    // We deliberately exclude containers that contain types without == (ImageGroup, QPixmap)
    // and also exclude pendingGroups and thumbnailWaiters, which are transient.
    return phaseProgress == other.phaseProgress &&
           statusMessage == other.statusMessage &&
           pipelineState == other.pipelineState;
}
} // namespace photoboss
