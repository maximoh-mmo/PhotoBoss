#pragma once
#include <QObject>
#include "util/Queue.h"
#include "util/FileTypes.h"
#include "pipeline/Pipeline.h"

namespace photoboss {
    class ResultProcessor : public PipelineStage
    {
        Q_OBJECT
    public:
        explicit ResultProcessor(Queue<std::shared_ptr<HashedImageResult>>& queue, QObject* parent = nullptr);
        void stop();
    public slots:
        void Run();

    signals:
        void imageHashed(std::shared_ptr <HashedImageResult> imageHash);

    private:
		Queue<std::shared_ptr<HashedImageResult>>& m_input;
    };
}
