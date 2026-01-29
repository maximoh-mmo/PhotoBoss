#pragma once
#include <QObject>
/// <summary>
/// 
/// Base for a stage in the processing pipeline.
/// All stages should inherit from this class and implement the Run() method.
/// 
/// Signals:
/// 
/// - status(const QString& message): Emitted to report status messages.
/// - progress(qint64 current, qint64 total): Emitted to report progress of the stage.
/// - error(const QString& message): Emitted to report errors encountered during processing.
/// 
/// </summary>

namespace photoboss {

    class StageBase : public QObject {
        Q_OBJECT
    public:
        explicit StageBase(QString id, QObject* parent = nullptr)
            : m_id(std::move(id)), QObject(parent) {
        }

        ~StageBase() override = default;
    
        QString stageId() const { return m_id; }

    public slots:
        // Main execution loop for the stage.
        // Must exit when its input queue is shut down.
        virtual void Run() = 0;

    signals:
        void status(const QString& message);
        void progress(qint64 current, qint64 total);
        void error(const QString& message);
    
    private:
        QString m_id;
    };
}
