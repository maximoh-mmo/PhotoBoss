#include "pipeline/stages/DiskReader.h"
#include "types/DataTypes.h"
#include "exif/ExifParser.h"
#include <QCryptographicHash>
#include <QFile>
#include <QThread>


namespace photoboss {

DiskReader::DiskReader(Queue<FileIdentity> &input_queue,
                       Queue<std::unique_ptr<DiskReadResult>> &queue,
                       QObject *parent)
    : StageBase(parent), m_input_queue_(input_queue), m_output_queue_(queue) {
  m_output_queue_.register_producer();
}

void DiskReader::run() {
  while (true) {
    FileIdentity fileIdentity;
    if (!m_input_queue_.wait_and_pop(fileIdentity)) {
      break;
    }

    QFile file(fileIdentity.path() + "/" + fileIdentity.name());
    if (file.open(QIODevice::ReadOnly)) {
      QByteArray bytes = file.readAll();
      ExifData exif = exif::ExifParser::parse(bytes);
      FileIdentity fullId(
          fileIdentity.name(), fileIdentity.path(),
          fileIdentity.extension(), fileIdentity.size(),
          fileIdentity.modifiedTime(), exif);
      auto result =
          std::make_unique<DiskReadResult>(std::move(fullId), std::move(bytes));

      if (!m_output_queue_.push(std::move(result))) {
        qDebug()
            << "DiskReader: Output queue shutdown, file dropped, stopping.";
        return;
      }
    }
  }
}

void DiskReader::onStop() { m_output_queue_.producer_done(); }
} // namespace photoboss