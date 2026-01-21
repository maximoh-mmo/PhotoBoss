#pragma once
#include <QObject>

namespace photoboss {

    class PipelineStage : public QObject {
        Q_OBJECT
    public:
        explicit PipelineStage(QObject* parent = nullptr)
            : QObject(parent) {
        }

        ~PipelineStage() override = default;

    public slots:
        // Main execution loop for the stage.
        // Must exit when its input queue is shut down.
        virtual void Run() = 0;
    };

} // namespace photoboss#pragma once
