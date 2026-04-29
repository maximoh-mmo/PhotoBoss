#include "Pipeline.h"
#include "caching/SqliteHashCache.h"

photoboss::Pipeline::Pipeline(QObject* parent)
{
	Token t;
	m_scanId_ = SqliteHashCache(0).nextScanId(t);
}

photoboss::Pipeline::~Pipeline()
{
}

void photoboss::Pipeline::clearQueues()
{
    for (IQueue* q : allQueues) {
        q->clear();
    }
}

void photoboss::Pipeline::requestShutdown()
{
	Token t;
    for (IQueue* q : allQueues) {
        q->request_shutdown(t);
	}
}
