#pragma once
#include <QObject>
#include "util/Queue.h"
#include "util/FileTypes.h"
#include "pipeline/Pipeline.h"
#include "caching/IHashCache.h"
#include "hashing/HashRegistry.h"

namespace photoboss
{
    class CacheLookup : public PipelineStage {
        Q_OBJECT
    public:
        CacheLookup(
            std::shared_ptr<Queue<FingerprintBatchPtr>>& input,
            std::shared_ptr<Queue<CacheLookupResult>>& output,
            IHashCache& cache,
            const std::vector<HashRegistry::Entry>& activeMethods,
            QObject* parent = nullptr
        );

        void Run() override;

    private:
        std::shared_ptr<Queue<FingerprintBatchPtr>>& m_input;
        std::shared_ptr<Queue<CacheLookupResult>>& m_output;
        IHashCache& m_cache;
        QList<QString> m_activeHashMethods;
        
	};
}
