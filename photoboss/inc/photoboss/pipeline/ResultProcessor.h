#pragma once
#include <QObject>
#include "util/Queue.h"
#include "util/DataTypes.h"
#include "pipeline/stages/Pipeline.h"

namespace photoboss {
    class ResultProcessor : public Sink<std::shared_ptr<HashedImageResult>>
    {
        Q_OBJECT
    public:
        explicit ResultProcessor(Queue<std::shared_ptr<HashedImageResult>>& queue,
            QString id, QObject* parent = nullptr);

        // Inherited via Sink
        void consume(const std::shared_ptr<HashedImageResult>& item) override;

    signals:
        void imageHashed(std::shared_ptr<HashedImageResult> imageHash);
    };
}