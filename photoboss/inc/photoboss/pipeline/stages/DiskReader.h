#pragma once

#include <list>
#include <memory>
#include "types/DataTypes.h"
#include "util/Queue.h"
#include "pipeline/StageBase.h"

namespace photoboss {

    class DiskReader : public StageBase {
        Q_OBJECT
    public:
        explicit DiskReader(
			Queue<FileIdentityBatchPtr>& input,
            Queue<std::unique_ptr<DiskReadResult>>& output, 
            QObject* parent = nullptr
        );

        void run();

    signals:
        void Finished();
        void ReadProgress(int current, int total);

    private:
        Queue<FileIdentityBatchPtr>& m_input_queue_; 
        Queue<std::unique_ptr<DiskReadResult>>& m_output_queue_;


        // Inherited via StageBase
        void onStop() override;

    };
}
