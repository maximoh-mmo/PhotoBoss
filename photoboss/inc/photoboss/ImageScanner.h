#pragma once
#include <qdatetime.h>
#include <QObject>
namespace photoboss
{
    typedef struct
    {
        QString path;
        quint64 size;
        QDateTime lastModified;
        QString suffix;
    } ImageFileMetaData;

    class ImageScanner : public QObject
    {
        Q_OBJECT
    public:
        explicit ImageScanner(QObject* parent = nullptr);
        void Cancel();

    public slots:
        void StartScan(const QString& directoryPath, bool recursive);
        
    signals:
        void filePathsCollected(const std::unique_ptr<std::list<ImageFileMetaData>>& meta_data);
        void fileCount(int);
        void progressUpdated(int);
        void scanCancelled();
        
    private:
        std::atomic<bool> m_cancelled_ = false;
    };
}
    