#pragma once

#include <QImage>
#include <optional>
#include <vector>

#include "types/DataTypes.h"   // DiskReadResult, FileIdentity, etc.

namespace photoboss {

/**
 * Very small helper that knows how to turn a DiskReadResult (raw bytes) into a
 * properly oriented QImage.  It purposefully does **no** hashing – it only
 * decodes the image and applies the EXIF orientation.  All heavy‑weight work
 * (hash computation) lives in HashEngine.
 */
class ImageLoader {
public:
    ImageLoader() = default;

    // Decode a single result.  Returns std::nullopt if the image cannot be read.
    std::optional<QImage> load(const DiskReadResult &item) const;

    // Decode a whole batch (vector of pointers to results).  Returns a vector
    // with the same ordering; each entry is either a valid QImage or nullopt.
    std::vector<std::optional<QImage>> loadBatch(const std::vector<DiskReadResult*>& batch) const;
};

} // namespace photoboss::pipeline::factory
