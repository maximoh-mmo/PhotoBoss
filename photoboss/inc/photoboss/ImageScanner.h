#pragma once
#include <qdatetime.h>
#include <QObject>

#include "DataTypes.h"

namespace photoboss
{
    
    class ImageScanner : public QObject
    {
        Q_OBJECT
    public:
        explicit ImageScanner(QObject* parent = nullptr);
        ~ImageScanner();
        void Cancel();

    public slots:
        void StartScan(const QString& directoryPath, bool recursive);
        
    signals:
        void filePathsCollected(const std::list<ImageFileMetaData>& meta_data);
        void scanned_file_count(int);
        void progressUpdated(int);
        void scanCancelled();
        
    private:
        std::atomic<bool> m_cancelled_ = false;
    };
}
    