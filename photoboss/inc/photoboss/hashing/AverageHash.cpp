#include "AverageHash.h"
namespace photoboss {
    QString AverageHash::compute(const PerceptualImage& image)
    {
        uint64_t hash = 0;
        int idx = 0;

        // Compute average pixel
        double sum = 0.0;
        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 8; ++x) {
                sum += image.pixel(x, y);
            }
        }
        double avg = sum / 64.0;

        // Generate hash
        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 8; ++x) {
                double pix = image.pixel(x, y);
                if (pix >= avg) {
                    hash |= (1ULL << idx);
                }
                ++idx;
            }
        }

        return QString("%1")
            .arg(hash, 16, 16, QChar('0'))
            .toUpper();
    }

    double AverageHash::compare(const QString& hash1, const QString& hash2) const
    {
        constexpr double maxDistance = 64.0;
        uint64_t a = hash1.toULongLong(nullptr, 16);
        uint64_t b = hash2.toULongLong(nullptr, 16);
        uint64_t diff = a ^ b;
        double distance = static_cast<double>(std::popcount(diff));
        return std::clamp(1.0 - (distance / maxDistance), 0.0, 1.0);
    }

    HashInput AverageHash::InputType() const
    {
        return HashInput::Image;
    }

}