#include "pipeline/stages/ImageLoader.h"
#include "util/AppSettings.h"
#include "util/OrientImage.h"
#include <QImageReader>
#include <QBuffer>

namespace photoboss {

std::optional<QImage> ImageLoader::load(const DiskReadResult &item, int targetSize) const {
    const FileIdentity &fi = item.fileIdentity;
    int size = targetSize > 0 ? targetSize : settings::HashSampleSize;
    QBuffer buf(const_cast<QByteArray*>(&item.imageBytes));
    buf.open(QIODevice::ReadOnly);
    QImageReader reader(&buf);
    reader.setScaledSize(QSize(size, size));
    QImage img = reader.read();
    if (img.isNull()) {
        return std::nullopt;
    }
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

} // namespace photoboss
