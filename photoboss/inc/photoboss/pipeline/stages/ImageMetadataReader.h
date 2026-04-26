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

class ImageMetadataReader : public StageBase {
    Q_OBJECT
public:
    explicit ImageMetadataReader(
        Queue<std::shared_ptr<QStringList>>& inputQueue,
        Queue<FileIdentityBatchPtr>& outputQueue,
        QObject* parent = nullptr);

    ~ImageMetadataReader() override;

private:
    void run() override;
    void onStop() override;

    std::atomic<bool> m_cancelled_{ false };
    Queue<std::shared_ptr<QStringList>>& m_inputQueue_;
    Queue<FileIdentityBatchPtr>& m_outputQueue_;
};

}