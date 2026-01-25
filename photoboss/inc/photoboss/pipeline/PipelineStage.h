#pragma once
#include <QObject>

namespace photoboss {

    class PipelineStage : public QObject {
        Q_OBJECT
    public:
        explicit PipelineStage(QObject* parent = nullptr)
            : QObject(parent) {
		}
        virtual void Run() = 0;
		virtual ~PipelineStage() = default;
    };
}