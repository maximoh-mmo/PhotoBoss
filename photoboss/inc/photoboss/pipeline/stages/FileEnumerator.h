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
        Queue<FileIdentity>& outputQueue,
        QObject* parent = nullptr);

    ~FileEnumerator() override;

private:
        void doRun() override;
    void onStop() override;

    std::atomic<bool> m_cancelled_{ false };
    ScanRequest m_request_;
    Queue<FileIdentity>& m_outputQueue_;
};

}