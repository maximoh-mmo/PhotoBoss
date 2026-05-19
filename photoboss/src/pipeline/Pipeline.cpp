#include "pipeline/Pipeline.h"
#include "caching/SqliteHashCache.h"

photoboss::Pipeline::Pipeline(QObject* parent)
{
	Token t;
	m_scanId_ = SqliteHashCache(0).nextScanId(t);
}

photoboss::Pipeline::~Pipeline()
{
    emit stateChanged(PipelineState::Stopping);

    for (QThread* thread : allThreads) {
        thread->quit();
        thread->wait(5000);
        if (thread->isRunning()) {
            qWarning() << "Thread" << thread << "did not finish in time";
        }
    }

    requestShutdown();
    emit stateChanged(PipelineState::Stopped);
}

void photoboss::Pipeline::AddThread(QThread* thread)
{
    connect(thread, &QThread::finished, this, &Pipeline::onThreadFinished);
    allThreads.push_back(std::move(thread));
}

void photoboss::Pipeline::onThreadFinished()
{
    m_runningThreads--;
    if (m_runningThreads <= 0 && state == PipelineState::Running) {
        state = PipelineState::Stopped;
        StageMetrics::instance().printAll();
        StageMetrics::instance().reset();
        emit stateChanged(PipelineState::Stopped);
    }
}

void photoboss::Pipeline::start()
{
    m_runningThreads = static_cast<int>(allThreads.size());
    for (QThread* thread : allThreads) {
        thread->start();
    }
    state = PipelineState::Running;
    emit stateChanged(PipelineState::Running);
}

void photoboss::Pipeline::stop()
{
    emit stateChanged(PipelineState::Stopping);
    clearQueues();
    requestShutdown();
    state = PipelineState::Stopped;
    emit stateChanged(PipelineState::Stopped);
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