#pragma once
#include <QObject>
#include <memory>
#include "util/Queue.h"
#include "types/DataTypes.h"
#include "pipeline/StageBase.h"
#include "caching/SqliteHashCache.h"

namespace photoboss {

    class ThumbnailGenerator : public StageBase
    {
        Q_OBJECT
    public:
        explicit ThumbnailGenerator(
            Queue<ThumbnailRequestPtr>& input,
            quint64 scanId,
            QObject* parent = nullptr
        );

    signals:
        void thumbnailReady(const ThumbnailResult& result);
        void workerFinished();

    protected:
        void doRun() override;
        void onStop() override;

    private:
        Queue<ThumbnailRequestPtr>& m_input_;
        std::unique_ptr<SqliteHashCache> m_cache_;
    };
}
