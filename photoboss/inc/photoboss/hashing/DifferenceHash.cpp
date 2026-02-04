#include "DifferenceHash.h"
#include "qtypes.h"
#include <bit>

namespace photoboss {
    QString DifferenceHash::compute(const PerceptualImage& image)
    {
        constexpr int startX = (64 - 9) / 2; // 27
        constexpr int startY = (64 - 8) / 2; // 28

        uint64_t hash = 0;
        
        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 8; ++x) {
                double left = image.pixel(startX + x, startY + y);
                double right = image.pixel(startX + x + 1, startY + y);

                int idx = y * 8 + x;
                if (left > right)
                    hash |= (1ULL << idx);
            }
        }

        return QString("%1")
            .arg(hash, 16, 16, QChar('0'))
            .toUpper();
    }

    double DifferenceHash::compare(const QString& hash1, const QString& hash2) const
    {
        constexpr double maxDistance = 64.0;
        uint64_t a = hash1.toULongLong(nullptr, 16);
        uint64_t b = hash2.toULongLong(nullptr, 16);
        uint64_t diff = a ^ b;
        double distance = static_cast<double>(std::popcount(diff));
        return std::clamp(1.0 - (distance / maxDistance), 0.0, 1.0);

    }

    HashInput DifferenceHash::InputType() const
    {
        return HashInput::Image;
    }
}