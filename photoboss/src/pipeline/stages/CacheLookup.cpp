#include "pipeline/stages/CacheLookup.h"
#include "caching/SqliteHashCache.h"

namespace photoboss
{
	CacheLookup::CacheLookup(Queue<FileIdentity>& input, Queue<FileIdentity>& diskOut, 
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
        int totalProcessed = 0;
        while (true) {
            FileIdentity fileId;

            if (!m_inputQueue_.wait_and_pop(fileId))
                break;

            totalProcessed++;
            emit incrementProgress(1);
            CacheQuery query(fileId);

            query.hashMethods = m_methods_;

            auto result = m_cache_->lookup(query);

            if (result.hit == Lookup::Hit) {
                m_resultQueue_.emplace(std::make_shared<HashedImageResult>(std::move(result.hashedImage)));
            }
            else {
                m_diskReadQueue_.emplace(std::move(fileId));
            }
        }
    }
}