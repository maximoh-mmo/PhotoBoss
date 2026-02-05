#pragma once
#include <QObject>
#include <QString>
#include <memory>
#include "util/DataTypes.h"
#include "util/Queue.h"
#include "pipeline/StageBase.h"

namespace photoboss {

    class DirectoryScanner : public StageBase
    {
        Q_OBJECT
    public:
		explicit DirectoryScanner(
            ScanRequest request,
            Queue<FileIdentityBatchPtr>& outputQueue,
            QObject* parent = nullptr);

        ~DirectoryScanner() override;

    private:
        std::atomic<bool> m_cancelled_{ false };
		ScanRequest m_request_;
        Queue<FileIdentityBatchPtr>& m_output;

        // Inherited via StageBase
        void run() override;
        void onStart() override;
        void onStop() override;
    };

}
