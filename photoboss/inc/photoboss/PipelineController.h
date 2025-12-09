#pragma once

#include <QObject>
#include <QThread>
#include <QString>
#include <QStringList>

namespace photoboss
{
    class DirectoryScanner;
    class ImageLoader;
    class ImageComparator;

    class PipelineController : public QObject
    {
        Q_OBJECT

    public:
        explicit PipelineController(QObject* parent = nullptr);
        ~PipelineController();

    public slots:
        void start(const QString& root_directory);
        void stop();

    signals:
        void progressUpdate(const QString& message);
        void fileGroupReady(const QStringList& group);
        void finished();

    private:
        void setupConnections();
        void setupThreads();

    private:
        QThread m_scanner_thread_;
        QThread m_loader_thread_;
        QThread m_compare_thread_;

        DirectoryScanner* m_scanner_ = nullptr;
        ImageLoader* m_loader_ = nullptr;
        ImageComparator* m_comparator_ = nullptr;

        bool m_stop_requested_ = false;
    };
}