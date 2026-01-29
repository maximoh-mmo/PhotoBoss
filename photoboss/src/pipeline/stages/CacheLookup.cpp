#include "stages/CacheLookup.h"
#include "util/FileTypes.h"

namespace photoboss
{
    CacheLookup::CacheLookup(std::shared_ptr<Queue<FingerprintBatchPtr>>& input, std::shared_ptr<Queue<CacheLookupResult>>& output, IHashCache& cache, const std::vector<HashRegistry::Entry>& activeMethods, QObject* parent)
        :
		PipelineStage(parent),
        m_input(input),
        m_output(output),
        m_cache(cache),
		m_activeHashMethods()
	{
        for (const auto& method : activeMethods) {
            m_activeHashMethods.push_back(method.key);
		}
    }
    void CacheLookup::Run()
    {
        FingerprintBatchPtr batch;
        
        for (auto method : m_activeHashMethods) {
            
		}

        while (m_input->wait_and_pop(batch)) {
            for (const auto& fingerprint : *batch) {
        
                CacheQuery query;
                query.fingerprint = fingerprint;
                query.requiredMethods = m_activeHashMethods;

                auto lookup = m_cache.lookup(query);

                CacheLookupResult out;
                out.hit = lookup.hit;
                out.hashedImage.fingerprint = fingerprint;

                if (lookup.hit == Route::CacheHit) {
                    out.hashedImage = std::move(lookup.hashedImage);
                }

                m_output->emplace(out);
            }
        }
		m_output->shutdown();
    }
}
