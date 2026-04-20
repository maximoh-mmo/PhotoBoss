#include "pipeline/stages/HashWorker.h"
#include "hashing/HashCatalog.h"
#include "hashing/HashMethod.h"
#include "util/OrientImage.h"
#include <QThread>
#include <qimage.h>
#include <qimagereader.h>


namespace photoboss {

HashWorker::HashWorker(Queue<std::unique_ptr<DiskReadResult>> &inputQueue,
                       Queue<std::shared_ptr<HashedImageResult>> &outputQueue,
                       QObject *parent)
    : StageBase("HashWorker", parent), m_input(inputQueue),
      m_output(outputQueue),
      m_hashMethods(std::move(HashCatalog::createAll())) {
  m_output.register_producer();
}

void HashWorker::run() {

  while (true) {
    std::unique_ptr<DiskReadResult> item;

    if (!m_input.wait_and_pop(item)) {
      break; // upstream shutdown
    }

    if (!item) {
      qDebug() << "HashWorker: Received null input, skipping.";
      continue;
    }
    QImageReader reader(item->fileIdentity.path() + "/" +
                        item->fileIdentity.name());

    auto result = std::make_shared<HashedImageResult>(
        item->fileIdentity, HashSource::Fresh, QDateTime::currentDateTimeUtc(),
        reader.size());

    for (auto &hash : m_hashMethods) {
      if (hash.method->InputType() == HashInput::Bytes) {
        try {
          result->hashes.emplace(hash.method->key(),
                                 hash.method->compute(item->imageBytes));
        } catch (const std::exception &e) {
          result->hashes.emplace(hash.method->key(), e.what());
          result->source = HashSource::Error;
          continue;
        }
      }
    }

    QImage img;
    img.loadFromData(item->imageBytes);
    img = OrientImage(img, item->fileIdentity.exif().orientation.value_or(1));

    if (img.isNull()) {
      qDebug() << "Image load failed" << item->fileIdentity.path();
      result->source = HashSource::Error;
    }

    else {
      for (auto &method : m_hashMethods) {
        if (method.method->InputType() == HashInput::Image) {
          try {
            result->hashes.emplace(
                method.method->key(),
                method.method->compute(PerceptualImage(img)));
          } catch (const std::exception &e) {
            result->hashes.emplace(method.method->key(), e.what());
            result->source = HashSource::Error;
          }
        }
      }
    }

    m_output.emplace(std::move(result));
  }
}

void HashWorker::onStop() { m_output.producer_done(); }
} // namespace photoboss
