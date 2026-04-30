#include "pipeline/HashEngine.h"
#include <QDebug>

namespace photoboss {

HashEngine::HashEngine(std::vector<HashCatalog::Entry> methods) {
    // Separate the methods by the type of input they require.
    for (auto &entry : methods) {
        if (entry.method->InputType() == HashInput::Bytes) {
            m_byteMethods.push_back(std::move(entry));
        } else if (entry.method->InputType() == HashInput::Image) {
            m_imageMethods.push_back(std::move(entry));
        }
    }
}

std::shared_ptr<HashedImageResult>
HashEngine::compute(const DiskReadResult &item, const std::optional<QImage> &image) const {
    // Initialise the result object – matches the legacy HashWorker constructor.
    auto result = std::make_shared<HashedImageResult>(
        item.fileIdentity,
        HashSource::Fresh,
        QDateTime::currentDateTimeUtc(),
        image ? image->size() : QSize{0,0}
    );

    // ---------- Byte‑based hash methods ----------
    for (const auto &entry : m_byteMethods) {
        try {
            result->hashes.emplace(entry.method->key(), entry.method->compute(item.imageBytes));
        } catch (const std::exception &e) {
            qDebug() << "HashEngine byte hash error:" << e.what();
            result->hashes.emplace(entry.method->key(), e.what());
            result->source = HashSource::Error;
        }
    }

    // ---------- Image‑based hash methods ----------
    if (image) {
        // image is valid – run each image‑hash method.
        for (const auto &entry : m_imageMethods) {
            try {
                result->hashes.emplace(entry.method->key(), entry.method->compute(PerceptualImage(*image)));
            } catch (const std::exception &e) {
                qDebug() << "HashEngine image hash error:" << e.what();
                result->hashes.emplace(entry.method->key(), e.what());
                result->source = HashSource::Error;
            }
        }
    } else {
        // No usable image – mark image‑based hashes as errors.
        for (const auto &entry : m_imageMethods) {
            result->hashes.emplace(entry.method->key(), QStringLiteral("decode_failed"));
            result->source = HashSource::Error;
        }
    }

    return result;
}

} // namespace photoboss::pipeline::factory
