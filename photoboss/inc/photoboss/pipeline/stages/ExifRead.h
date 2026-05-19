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

class ExifRead : public StageBase {
    Q_OBJECT
public:
    explicit ExifRead(
        Queue<QString>& inputQueue,
        Queue<FileIdentity>& outputQueue,
        QObject* parent = nullptr);

    ~ExifRead() override;

private:
    void run() override;
    void onStop() override;

    std::atomic<bool> m_cancelled_{ false };
    Queue<QString>& m_inputQueue_;
    Queue<FileIdentity>& m_outputQueue_;
};

}