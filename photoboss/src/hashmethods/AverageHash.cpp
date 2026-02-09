#include "hashing/AverageHash.h"
namespace photoboss {
    QString AverageHash::compute(const PerceptualImage& image)
    {
        // Sample offset to centre
        static constexpr int SampleWidth = 8;
        static constexpr int SampleHeight = 8;
        static constexpr int ImageSize = 32;

        constexpr int startX = (ImageSize - SampleWidth) / 2;
        constexpr int startY = (ImageSize - SampleHeight) / 2;

        double sum = 0.0;
                for (int y = 0; y < SampleHeight; ++y) {
            for (int x = 0; x < SampleWidth; ++x) {
                sum += image.pixel(startX + x, startY + y);
            }
        }
        double avg = sum / (SampleWidth * SampleHeight);
        
        uint64_t hash = 0;
        int idx = 0;
        
        // Generate hash
        for (int y = 0; y < SampleHeight; ++y) {
            for (int x = 0; x < SampleWidth; ++x) {
                double pix = image.pixel(startX + x, startY + y);
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
        constexpr double MaxBits = 32.0;
        uint64_t a = hash1.toULongLong(nullptr, 16);
        uint64_t b = hash2.toULongLong(nullptr, 16);
        uint64_t diff = a ^ b;
        double distance = static_cast<double>(std::popcount(diff));
        return std::clamp(1.0 - (distance / MaxBits), 0.0, 1.0);
    }

    HashInput AverageHash::InputType() const
    {
        return HashInput::Image;
    }

}