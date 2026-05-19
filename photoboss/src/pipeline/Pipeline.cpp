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

    for (QThread* thread : m_allThreads_) {
        thread->quit();
        thread->wait(5000);
        if (thread->isRunning()) {
            qWarning() << "Thread" << thread << "did not finish in time";
        }
    }

    requestShutdown();
    emit stateChanged(PipelineState::Stopped);
}

void photoboss::Pipeline::addThread(QThread* thread)
{
    connect(thread, &QThread::finished, this, &Pipeline::onThreadFinished);
    m_allThreads_.push_back(std::move(thread));
}

void photoboss::Pipeline::onThreadFinished()
{
    m_runningThreads_--;
    if (m_runningThreads_ <= 0 && m_state_ == PipelineState::Running) {
        m_state_ = PipelineState::Stopped;
        StageMetrics::instance().printAll();
        StageMetrics::instance().reset();
        emit stateChanged(PipelineState::Stopped);
    }
}

void photoboss::Pipeline::start()
{
    m_runningThreads_ = static_cast<int>(m_allThreads_.size());
    for (QThread* thread : m_allThreads_) {
        thread->start();
    }
    m_state_ = PipelineState::Running;
    emit stateChanged(PipelineState::Running);
}

void photoboss::Pipeline::stop()
{
    emit stateChanged(PipelineState::Stopping);
    clearQueues();
    requestShutdown();
    m_state_ = PipelineState::Stopped;
    emit stateChanged(PipelineState::Stopped);
}

void photoboss::Pipeline::clearQueues()
{
    for (auto& q : m_allQueues_) {
        q->clear();
    }
}

void photoboss::Pipeline::requestShutdown()
{
	Token t;
    for (auto& q : m_allQueues_) {
        q->request_shutdown(t);
	}
}