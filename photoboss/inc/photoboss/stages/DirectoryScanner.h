#pragma once
#include <QObject>
#include <QString>
#include <memory>
#include "util/DataTypes.h"
#include "pipeline/stages/Pipeline.h"

namespace photoboss {

    class DirectoryScanner : public Source<FileIdentityBatchPtr>
    {
        Q_OBJECT
    public:
		explicit DirectoryScanner(
            ScanRequest request,
            Queue<FileIdentityBatchPtr>& outputQueue,
            QObject* parent = nullptr);

        ~DirectoryScanner() override;

        // Inherited via Source
        void produce() override;
    private:
        std::atomic<bool> m_cancelled_{ false };
		ScanRequest m_request_;
    };

}
