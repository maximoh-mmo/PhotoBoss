#include "pipeline/CacheStore.h"
#include "caching/SqliteHashCache.h"
namespace photoboss
{
    CacheStore::CacheStore(
        Queue<std::shared_ptr<HashedImageResult>>& input,
        Queue<std::shared_ptr<HashedImageResult>>& output,
        QString id, QObject* parent
    ) :
        Transform<std::shared_ptr<HashedImageResult>, std::shared_ptr<HashedImageResult>>(input, output, id, parent),
        m_cache(std::make_unique<SqliteHashCache>())
    {
    }


    std::shared_ptr<HashedImageResult> photoboss::CacheStore::transform(const std::shared_ptr<HashedImageResult>& item)
    {
		qDebug() << "Storing hashes for file: %1" << item->fileIdentity.path();
        m_cache->store(HashedImageResult(item.get()->fileIdentity, item.get()->source, item.get()->cachedAt, item.get()->resolution, item.get()->hashes), {});
		return item;
    }
}