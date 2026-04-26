#pragma once
#include <QObject>
#include <QString>
#include <memory>
#include <vector>
#include <atomic>
#include "types/DataTypes.h"
#include "util/Queue.h"
#include "util/StorageInfo.h"
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
        Queue<FileIdentityBatchPtr>& m_output_;
        bool m_isFastStorage_{ false };
        std::vector<QString> m_filePaths_;
        std::vector<FileIdentity> m_fileIdentities_;

        void run() override;
        void onStop() override;

        int quickCountPhase();
        std::vector<FileIdentity> processFilesSequential(
            const std::vector<QString>& filePaths);
        std::vector<FileIdentity> processFilesParallel(
            const std::vector<QString>& filePaths);
    };

}
