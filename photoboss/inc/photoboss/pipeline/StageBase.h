#pragma once
#include <QObject>
#include <QElapsedTimer>
#include <QTimer>
#include "util/AppSettings.h"
/// <summary>
/// 
/// Base for a stage in the processing pipeline.
/// All stages should inherit from this class and implement the Run() method.
/// 
/// Signals:
/// 
/// - status(const QString& message): Emitted to report status messages.
/// - incrementProgress(int count): Emitted per item processed.
/// - finalCount(int total): Emitted when stage completes with total count.
/// - error(const QString& message): Emitted to report errors encountered during processing.
/// 
/// </summary>

namespace photoboss {

    class StageBase : public QObject {
        Q_OBJECT
    public:
        explicit StageBase(QObject* parent = nullptr)
            : QObject(parent) {
        }
        ~StageBase() override = default;
    
        void Run() {
            try {
                run();
            }
            catch (const std::exception& e) {
                error(e.what());
            }

            onStop();
            emit stageFinished();
        }
        // Main execution loop for the stage.
        // Must exit when its input queue is shut down.

    signals:
		// Emitted to report progress. 'count' is the number of items processed since the last update.
        void incrementProgress(int count);
		// Emitted when stage completes with total count of items processed.
        void finalCount(int total);
		// Emitted to report status messages.
        void status(const QString& message);
		// Emitted to report errors encountered during processing.
        void error(const QString& message);
        // Emitted when stage completes all work
        void stageFinished();

    protected:
        virtual void run() = 0;
        virtual void onStop() = 0;
    };
}
