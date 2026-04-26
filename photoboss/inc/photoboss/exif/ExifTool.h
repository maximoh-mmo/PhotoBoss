#pragma once
#include <exiv2/exiv2.hpp>
#include "types/ExifData.h"
#include <qstring.h>

namespace photoboss {
namespace exif {

class ExifTool {
public:
    static ExifData parse(const QString& filePath);

private:
    ExifTool() = delete;
    ~ExifTool() = delete;
};

}
}