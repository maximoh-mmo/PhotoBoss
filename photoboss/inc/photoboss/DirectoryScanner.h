#pragma once
#include <QObject>
#include <QString>
#include <memory>
#include "DataTypes.h"

namespace photoboss {

    class DirectoryScanner : public QObject {
        Q_OBJECT
    public:
        explicit DirectoryScanner(QObject* parent = nullptr);
        ~DirectoryScanner() override;

    public slots:
        void StartScan(const QString& directory, bool recursive);

    signals:
        void fileFound(const Fingerprint& meta);
        void fileBatchFound(FingerprintBatchPtr batch);
        void status(const QString& message);
        void finished();

    private:
        std::atomic<bool> m_cancelled_{ false };
    };

}
