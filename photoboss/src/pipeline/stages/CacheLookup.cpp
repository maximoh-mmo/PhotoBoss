#include "pipeline/stages/CacheLookup.h"
#include "caching/SqliteHashCache.h"

namespace photoboss
{
	CacheLookup::CacheLookup(Queue<FileIdentityBatchPtr>& input, Queue<FileIdentityBatchPtr>& diskOut, 
        Queue<std::shared_ptr<HashedImageResult>>& resultOut, quint64 scanId, QObject* parent)
		: StageBase(parent),
		m_inputQueue_(input),
		m_diskReadQueue_(diskOut),
		m_resultQueue_(resultOut),
		m_cache_(std::make_unique<SqliteHashCache>(scanId))
	{
        for (HashCatalog::Entry& method : HashCatalog::createAll()) {
            m_methods_.append(method.method.get()->key());
        }
        m_resultQueue_.register_producer();
        m_diskReadQueue_.register_producer();
	}


    void CacheLookup::onStop()
    {

        m_resultQueue_.producer_done();
        m_diskReadQueue_.producer_done();
    }

	void CacheLookup::run()
	{
        while (true) {
            FileIdentityBatchPtr batch;

            if (!m_inputQueue_.wait_and_pop(batch))
                break;

            if (!batch || batch->empty())
                continue;

            std::shared_ptr<std::vector<FileIdentity>> misses;
            misses = std::make_shared<std::vector<FileIdentity>>();
            misses->reserve(batch->size());

            for (const auto& fileId : *batch) {
                CacheQuery query(fileId);

                query.hashMethods = m_methods_; // empty means "any"

                auto result = m_cache_->lookup(query);

                if (result.hit == Lookup::Hit) {                   
                    m_resultQueue_.emplace(std::make_shared<HashedImageResult>(std::move(result.hashedImage)));
                }
                else {
                    misses->push_back(fileId);
                }
            }

            if (!misses->empty()) {
                m_diskReadQueue_.emplace(std::move(misses));
            }
        }
	}
}