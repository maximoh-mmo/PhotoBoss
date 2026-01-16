#pragma once

#include <list>
#include <memory>

#include "DataTypes.h"
#include "Queue.h"

namespace photoboss {

    class DiskReader : public QObject {
        Q_OBJECT
    public:
        explicit DiskReader(
			Queue<FileMetaListPtr>& input_queue,
            Queue<std::unique_ptr<DiskReadResult>>& queue, 
            QObject* parent = nullptr
        );

    public slots:
        void Run();

    signals:
        void Finished();
        void ReadProgress(int current, int total);

    private:
        Queue<FileMetaListPtr>& m_input_queue; 
        Queue<std::unique_ptr<DiskReadResult>>& m_output_queue;

    };
}
