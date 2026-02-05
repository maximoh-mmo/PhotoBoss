#include "pipeline/stages/CacheStore.h"
#include "caching/SqliteHashCache.h"
namespace photoboss
{
    CacheStore::CacheStore(
        Queue<std::shared_ptr<HashedImageResult>>& input,
        Queue<std::shared_ptr<HashedImageResult>>& output,
        QString id, QObject* parent
    ) :
        StageBase(id, parent),
        m_cache(std::make_unique<SqliteHashCache>()),
        m_output(output),
        m_input(input)
    {
    }

    void CacheStore::run()
    {
        std::shared_ptr<HashedImageResult> item;
        while (m_input.wait_and_pop(item)) {
            m_cache->store(HashedImageResult(item.get()->fileIdentity, item.get()->source, item.get()->cachedAt, item.get()->resolution, item.get()->hashes), {});
            m_output.push(std::move(item));
        }
    }

    void CacheStore::onStart()
    {
        qDebug() << "store registered";
        m_output.register_producer();
    }
    void CacheStore::onStop()
    {
        qDebug() << "store deregistered";
        m_output.producer_done();
    }
}