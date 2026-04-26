#pragma once
#include <exiv2/exiv2.hpp>
#include "types/ExifData.h"
#include <qstring.h>

namespace photoboss {
namespace exif {

class ExifParser {
public:
    static ExifData parse(const QString& filePath);

private:
    ExifParser() = delete;
    ~ExifParser() = delete;
};

}
}