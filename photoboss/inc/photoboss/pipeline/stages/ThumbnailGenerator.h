#pragma once
#include <QObject>
#include "util/Queue.h"
#include "types/DataTypes.h"
#include "pipeline/StageBase.h"

namespace photoboss {

    class ThumbnailGenerator : public StageBase
    {
        Q_OBJECT
    public:
        explicit ThumbnailGenerator(
            Queue<ThumbnailRequestPtr>& input,
            QObject* parent = nullptr
        );

    signals:
        void thumbnailReady(const ThumbnailResult& result);
        void workerFinished();

    protected:
        void run() override;
        void onStop() override;

    private:
        Queue<ThumbnailRequestPtr>& m_input_;
    };
}
