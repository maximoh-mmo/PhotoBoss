#include "pipeline/stages/CacheStore.h"
#include "caching/SqliteHashCache.h"
#include "util/AppSettings.h"

namespace photoboss
{
    CacheStore::CacheStore(
        Queue<std::shared_ptr<HashedImageResult>>& input,
        Queue<std::shared_ptr<HashedImageResult>>& output,
        quint64 scanId, QObject* parent
    ) :
        StageBase(parent),
        m_cache_(std::make_unique<SqliteHashCache>(scanId)),
        m_output_(output),
        m_input_(input)
    {
        m_output_.register_producer();
        m_batch_.reserve(settings::CacheStoreBatchSize);
    }

    void CacheStore::flushBatch()
    {
        if (m_batch_.empty()) return;
        m_cache_->storeBatch(m_batch_);
        m_batch_.clear();
        m_batch_.reserve(settings::CacheStoreBatchSize);
    }

    void CacheStore::run()
    {
        std::shared_ptr<HashedImageResult> item;
        while (m_input_.wait_and_pop(item)) {
            HashedImageResult copy(item->fileIdentity, item->source,
                item->cachedAt, item->resolution, item->hashes);
            copy.decodedImage = item->decodedImage;
            m_batch_.emplace_back(std::move(copy), QMap<QString, int>{});
            m_output_.push(std::move(item));
            if (m_batch_.size() >= settings::CacheStoreBatchSize)
                flushBatch();
        }
        flushBatch();
    }

    void CacheStore::onStop()
    {
        flushBatch();
        m_output_.producer_done();
    }
}