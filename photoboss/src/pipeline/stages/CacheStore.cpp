#include "pipeline/CacheStore.h"
namespace photoboss
{
    CacheStore::CacheStore(
        Queue<std::shared_ptr<HashedImageResult>>& input,
        Queue<std::shared_ptr<HashedImageResult>>& output,
        IHashCache& cache,
        const std::vector<HashRegistry::Entry>& activeMethods,
        QString id, QObject* parent
    ) :
        Transform<std::shared_ptr<HashedImageResult>, std::shared_ptr<HashedImageResult>>(input, output, id, parent),
        m_activeHashMethods(activeMethods),
        m_cache(cache)
    {
    }


    std::shared_ptr<HashedImageResult> photoboss::CacheStore::transform(const std::shared_ptr<HashedImageResult>& item)
    {
		qDebug() << "Storing hashes for file: %1" << item->fileIdentity.path();
        m_cache.store(HashedImageResult(item.get()->fileIdentity, item.get()->source, item.get()->cachedAt, item.get()->hashes), {});
		return item;
    }
}