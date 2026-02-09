#include "pipeline/stages/CacheLookup.h"
#include "caching/SqliteHashCache.h"

namespace photoboss
{
	CacheLookup::CacheLookup(Queue<FileIdentityBatchPtr>& input, Queue<FileIdentityBatchPtr>& diskOut, 
        Queue<std::shared_ptr<HashedImageResult>>& resultOut, QString id, QObject* parent)
		: StageBase(id, parent),
		m_inputQueue(input),
		m_diskReadQueue(diskOut),
		m_resultQueue(resultOut),
		m_cache(std::make_unique<SqliteHashCache>())
	{
        for (HashCatalog::Entry& method : HashCatalog::createAll()) {
            m_methods.append(method.method.get()->key());
        }
	}

    void CacheLookup::onStart()
    {
        m_resultQueue.register_producer();
        m_diskReadQueue.register_producer();
    }

    void CacheLookup::onStop()
    {

        m_resultQueue.producer_done();
        m_diskReadQueue.producer_done();
    }

	void CacheLookup::run()
	{
        while (true) {
            FileIdentityBatchPtr batch;

            if (!m_inputQueue.wait_and_pop(batch))
                break;

            if (!batch || batch->empty())
                continue;

            std::shared_ptr<std::vector<FileIdentity>> misses;
            misses = std::make_shared<std::vector<FileIdentity>>();
            misses->reserve(batch->size());

            for (const auto& fileId : *batch) {
                CacheQuery query(fileId);

                query.hashMethods = m_methods; // empty means "any"

                auto result = m_cache->lookup(query);

                if (result.hit == Lookup::Hit) {                   
                    m_resultQueue.emplace(std::make_shared<HashedImageResult>(std::move(result.hashedImage)));
                }
                else {
                    misses->push_back(fileId);
                }
            }

            if (!misses->empty()) {
                m_diskReadQueue.emplace(std::move(misses));
            }
        }
	}
}