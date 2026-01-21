#include "CacheLookup.h"
namespace photoboss
{
	CacheLookup::CacheLookup(Queue<FingerprintBatchPtr>& inputQueue, Queue<FingerprintBatchPtr>& diskReadQueue, QObject* parent)
		: QObject(parent),
		m_inputQueue(inputQueue),
		m_diskReadQueue(diskReadQueue)
	{
	}

	void CacheLookup::Run()
	{
		while (true)
		{
			FingerprintBatchPtr batch;
			if (!m_inputQueue.wait_and_pop(batch))
			{
				// Shutdown signaled
				break;
			}
			// Here we would check the cache for each file in the batch
			// For simplicity, we assume none are found in cache and forward all to disk read queue
			m_diskReadQueue.push(std::move(batch));
		}
	}
}