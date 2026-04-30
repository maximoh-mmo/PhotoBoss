#include "exif/ExifParser.h"
#include <QDateTime>

namespace photoboss {
namespace exif {

ExifData ExifParser::parse(const QString& filePath)
{
    ExifData result;

    try {
        auto image = Exiv2::ImageFactory::open(filePath.toStdString());
        if (!image) {
            return result;
        }

        image->readMetadata();
        const auto& exifData = image->exifData();

        if (exifData.empty()) {
            return result;
        }

        auto itOrientation = exifData.findKey(Exiv2::ExifKey("Exif.Image.Orientation"));
        if (itOrientation != exifData.end()) {
            const qint64 value = itOrientation->toInt64();
            if (value >= 1 && value <= 8) {
                result.orientation = static_cast<int>(value);
            }
        }

        auto itDate = exifData.findKey(Exiv2::ExifKey("Exif.Photo.DateTimeOriginal"));
        if (itDate != exifData.end()) {
            const std::string& dateStr = itDate->toString();
            QDateTime dt = QDateTime::fromString(
                QString::fromStdString(dateStr),
                "yyyy:MM:dd HH:mm:ss"
            );
            if (dt.isValid()) {
                result.dateTimeOriginal = static_cast<quint64>(dt.toSecsSinceEpoch());
            }
        }

        auto itMake = exifData.findKey(Exiv2::ExifKey("Exif.Image.Make"));
        if (itMake != exifData.end()) {
            result.cameraMake = QString::fromStdString(itMake->toString()).trimmed();
        }

        auto itModel = exifData.findKey(Exiv2::ExifKey("Exif.Image.Model"));
        if (itModel != exifData.end()) {
            result.cameraModel = QString::fromStdString(itModel->toString()).trimmed();
        }
    }
    catch (const Exiv2::Error&) {
    }

    return result;
}

}
}