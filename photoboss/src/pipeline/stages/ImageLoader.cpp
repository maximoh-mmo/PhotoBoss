#include "pipeline/stages/ImageLoader.h"
#include "util/OrientImage.h"
#include <QImageReader>
#include <QFileInfo>

namespace photoboss {

std::optional<QImage> ImageLoader::load(const DiskReadResult &item) const {
    // Build the full path from the FileIdentity stored in DiskReadResult.
    const FileIdentity &fi = item.fileIdentity;
    QString fullPath = fi.path() + "/" + fi.name();
    QImageReader reader(fullPath);
    QImage img = reader.read();
    if (img.isNull()) {
        return std::nullopt; // decode failure – let the caller handle the error.
    }
    // Apply EXIF orientation if it exists; default is 1 (no rotation).
    int orientation = fi.exif().orientation.value_or(1);
    img = OrientImage(img, orientation);
    return img;
}

std::vector<std::optional<QImage>> ImageLoader::loadBatch(const std::vector<DiskReadResult*> &batch) const {
    std::vector<std::optional<QImage>> results;
    results.reserve(batch.size());
    for (auto *ptr : batch) {
        if (!ptr) {
            results.emplace_back(std::nullopt);
            continue;
        }
        results.emplace_back(load(*ptr));
    }
    return results;
}

} // namespace photoboss::pipeline::factory
