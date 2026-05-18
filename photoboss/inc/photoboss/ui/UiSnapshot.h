#pragma once

#include <QMap>
#include <QMultiMap>
#include <QPixmap>
#include <deque>
#include <utility>

#include "types/GroupTypes.h"
#include "types/DataTypes.h"
#include "pipeline/Pipeline.h"

namespace photoboss {

class ImageThumbWidget;

struct UiSnapshot {
    std::deque<ImageGroup> pendingGroups;
    QMap<quint64, ImageGroup> updatedGroups;
    QMap<QString, QPixmap> thumbnailCache;
    QMultiMap<QString, ImageThumbWidget*> thumbnailWaiters;
    QMap<Pipeline::Phase, std::pair<int,int>> phaseProgress;
    QString statusMessage;
    Pipeline::PipelineState pipelineState = Pipeline::PipelineState::Stopped;

    bool operator==(const UiSnapshot& other) const;
};

} // namespace photoboss
