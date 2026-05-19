#pragma once
#include <exiv2/exiv2.hpp>
#include "types/ExifData.h"
#include <QString>

namespace photoboss {
namespace exif {

class ExifParser {
public:
    static ExifData parse(const QString& filePath);
    static ExifData parse(const QByteArray& bytes);

private:
    static ExifData parseFromImage(Exiv2::Image::UniquePtr image);

    ExifParser() = delete;
    ~ExifParser() = delete;
};

}
}