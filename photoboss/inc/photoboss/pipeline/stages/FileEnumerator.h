#pragma once
#include <QObject>
#include <QString>
#include <memory>
#include <vector>
#include <atomic>
#include "types/DataTypes.h"
#include "util/Queue.h"
#include "pipeline/StageBase.h"

namespace photoboss {

class FileEnumerator : public StageBase {
    Q_OBJECT
public:
    explicit FileEnumerator(
        ScanRequest request,
        Queue<std::shared_ptr<QStringList>>& outputQueue,
        QObject* parent = nullptr);

    ~FileEnumerator() override;

private:
    void run() override;
    void onStop() override;

    std::atomic<bool> m_cancelled_{ false };
    ScanRequest m_request_;
    Queue<std::shared_ptr<QStringList>>& m_outputQueue_;
};

}