#pragma once

#include <list>
#include <memory>
#include <QImage>

#include "DataTypes.h"
#include "Queue.h"

namespace photoboss {

    class DiskReader : public QObject {
        Q_OBJECT
    public:
        explicit DiskReader(Queue<std::unique_ptr<DiskReadResult>>& queue, QObject* parent = nullptr);

    public slots:
        void Start(const std::unique_ptr<std::list<ImageFileMetaData>>& files);
        void Cancel();

    signals:
        void Finished();
        void ReadProgress(int current, int total);

    private:
        std::atomic<bool> m_cancelled_;
        Queue<std::unique_ptr<DiskReadResult>>& m_output_queue;
    };
}
