#pragma once

#include <list>
#include <memory>
#include "DataTypes.h"
#include "Queue.h"
#include "PipelineStage.h"

namespace photoboss {

    class DiskReader : public PipelineStage {
        Q_OBJECT
    public:
        explicit DiskReader(
			Queue<FingerprintBatchPtr>& input_queue,
            Queue<std::unique_ptr<DiskReadResult>>& queue, 
            QObject* parent = nullptr
        );

    public slots:
        void Run();

    signals:
        void Finished();
        void ReadProgress(int current, int total);

    private:
        Queue<FingerprintBatchPtr>& m_input_queue; 
        Queue<std::unique_ptr<DiskReadResult>>& m_output_queue;

    };
}
