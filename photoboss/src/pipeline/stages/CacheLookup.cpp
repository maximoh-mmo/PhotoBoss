#include "pipeline/CacheLookup.h"
namespace photoboss
{
	CacheLookup::CacheLookup(Queue<FileIdentityBatchPtr>& input, Queue<FileIdentityBatchPtr>& diskOut, 
        Queue<std::shared_ptr<HashedImageResult>>& resultOut, IHashCache& cache, 
        const std::vector<HashRegistry::Entry>& activeMethods, QString id, QObject* parent)
		: StageBase(id, parent),
		m_inputQueue(input),
		m_diskReadQueue(diskOut),
		m_resultQueue(resultOut),
		m_cache(cache),
		m_activeHashMethods(activeMethods)
	{
	}

	void CacheLookup::Run()
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
                auto requiredMethods = QList<QString>();
                for (auto method : m_activeHashMethods) {
                    requiredMethods.append(method.key);
                }
                query.requiredMethods = requiredMethods; // empty means "any"

                auto result = m_cache.lookup(query);

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
        m_inputQueue.shutdown();
	}
}