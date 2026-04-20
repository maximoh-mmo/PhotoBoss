#include "pipeline/stages/CacheStore.h"
#include "caching/SqliteHashCache.h"

namespace photoboss
{
    CacheStore::CacheStore(
        Queue<std::shared_ptr<HashedImageResult>>& input,
        Queue<std::shared_ptr<HashedImageResult>>& output,
        QString id, quint64 scanId, QObject* parent
    ) :
        StageBase(id, parent),
        m_cache_(std::make_unique<SqliteHashCache>(scanId)),
        m_output_(output),
        m_input_(input)
    {
        m_output_.register_producer();
    }

    void CacheStore::run()
    {
        std::shared_ptr<HashedImageResult> item;
        while (m_input_.wait_and_pop(item)) {
            m_cache_->store(HashedImageResult(item.get()->fileIdentity, item.get()->source, item.get()->cachedAt, item.get()->resolution, item.get()->hashes), {});
            m_output_.push(std::move(item));
        }
    }

    void CacheStore::onStop()
    {
        m_output_.producer_done();
    }
}