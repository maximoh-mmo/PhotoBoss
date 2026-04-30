#include "Pipeline.h"
#include "caching/SqliteHashCache.h"

photoboss::Pipeline::Pipeline(QObject* parent)
{
	Token t;
	m_scanId_ = SqliteHashCache(0).nextScanId(t);
}

photoboss::Pipeline::~Pipeline()
{
    // Request shutdown first to stop producers
    requestShutdown();

    // Wait for all threads to finish
    for (QThread* thread : allThreads) {
        thread->quit();
        thread->wait(5000); // Wait up to 5 seconds
        if (thread->isRunning()) {
            qWarning() << "Thread" << thread << "did not finish in time";
        }
    }
}

void photoboss::Pipeline::start()
{
    for (QThread* thread : allThreads) {
        thread->start();
    }
    state = PipelineState::Running;
}

void photoboss::Pipeline::stop()
{
    clearQueues();
    requestShutdown();
    state = PipelineState::Stopped;
}

void photoboss::Pipeline::clearQueues()
{
    for (auto& q : allQueues) {
        q->clear();
    }
}

void photoboss::Pipeline::requestShutdown()
{
	Token t;
    for (auto& q : allQueues) {
        q->request_shutdown(t);
	}
}
