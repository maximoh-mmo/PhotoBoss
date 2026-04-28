#pragma once

#include <memory>
#include <vector>
#include <optional>
#include <QImage>

#include "hashing/HashCatalog.h"
#include "types/DataTypes.h"   // HashedImageResult, DiskReadResult, HashSource, etc.

namespace photoboss::pipeline::factory {

/**
 * Pure‑logic class that knows which hash methods require raw bytes and which
 * require a QImage.  It receives a DiskReadResult (raw bytes + metadata) and an
 * optional QImage (produced by ImageLoader) and returns a fully populated
 * HashedImageResult.
 */
class HashEngine {
public:
    explicit HashEngine(std::vector<HashCatalog::Entry> methods);

    // Compute hashes for a single file.  The optional QImage may be empty on
    // decode failure – in that case image‑based hashes are skipped and the
    // result source is marked as Error.
    std::shared_ptr<HashedImageResult>
    compute(const DiskReadResult &item, const std::optional<QImage> &image) const;

private:
    std::vector<HashCatalog::Entry> m_byteMethods;
    std::vector<HashCatalog::Entry> m_imageMethods;
};

} // namespace photoboss::pipeline::factory
