#pragma once

#include <list>
#include <memory>
#include "util/DataTypes.h"
#include "util/Queue.h"
#include "pipeline/stages/Pipeline.h"

namespace photoboss {

    class DiskReader : public StageBase {
        Q_OBJECT
    public:
        explicit DiskReader(
			Queue<FileIdentityBatchPtr>& input,
            Queue<std::unique_ptr<DiskReadResult>>& output, 
            QObject* parent = nullptr
        );

    public slots:
        void Run();

    signals:
        void Finished();
        void ReadProgress(int current, int total);

    private:
        Queue<FileIdentityBatchPtr>& m_input_queue; 
        Queue<std::unique_ptr<DiskReadResult>>& m_output_queue;

    };
}
