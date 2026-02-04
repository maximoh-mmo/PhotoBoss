#pragma once
#include <exiv2/exiv2.hpp>
#include "util/ExifData.h"
#include <qstring.h>

namespace exif
{
	class ExifReader {
	public:
		static photoboss::ExifData read(const QString& filePath);
	};
}