#include "CacheLookup.h"
namespace photoboss
{
	CacheLookup::CacheLookup(Queue<FingerprintBatchPtr>& input, Queue<FingerprintBatchPtr>& diskOut, 
        Queue<std::shared_ptr<HashedImageResult>>& resultOut, IHashCache& cache, 
        const std::vector<HashRegistry::Entry>& activeMethods, QObject* parent)
		: PipelineStage(parent),
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
            FingerprintBatchPtr batch;

            if (!m_inputQueue.wait_and_pop(batch))
                break;

            if (!batch || batch->empty())
                continue;

            std::shared_ptr<std::vector<Fingerprint>> misses;
            misses = std::make_shared<std::vector<Fingerprint>>();
            misses->reserve(batch->size());

            for (const auto& fingerprint : *batch) {
                CacheQuery query;
                query.fingerprint = fingerprint;
                auto requiredMethods = QList<QString>();
                for (auto method : m_activeHashMethods) {
                    requiredMethods.append(method.key);
                }
                query.requiredMethods = requiredMethods; // empty means "any"

                auto result = m_cache.lookup(query);

                if (result.hit) {
                    auto cached = std::make_shared<HashedImageResult>();
                    cached->fingerprint = fingerprint;
                    cached->hashes = result.hashedImage.hashes;
                    cached->source = HashSource::Cache;

                    m_resultQueue.emplace(cached);
                }
                else {
                    misses->push_back(fingerprint);
                }
            }

            if (!misses->empty()) {
                m_diskReadQueue.emplace(std::move(misses));
            }
        }
	}
}