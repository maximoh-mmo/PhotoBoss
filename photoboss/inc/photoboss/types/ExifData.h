#pragma once
#include <optional>
#include <qtypes.h>
#include <qstring.h>

namespace photoboss {
	struct ExifData {
		std::optional<int> orientation;
		std::optional<quint64> dateTimeOriginal;
		std::optional<QString> cameraMake;
		std::optional<QString> cameraModel;

		bool operator==(const ExifData& other) const noexcept {
			auto matchOptional = [](auto& a, auto& b) {
				return !a || !b || a == b;
				};

			return matchOptional(orientation, other.orientation) &&
				matchOptional(dateTimeOriginal, other.dateTimeOriginal) &&
				matchOptional(cameraMake, other.cameraMake) &&
				matchOptional(cameraModel, other.cameraModel);
		}
	};
}