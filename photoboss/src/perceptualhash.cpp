#include <QString>
#include <QtMath>
#include "HashMethod.h"
namespace photoboss
{
    const bool ph_registered =
        photoboss::HashRegistry::registerFactory<photoboss::PerceptualHash>(QStringLiteral("Perceptual Hash"));

    QString PerceptualHash::computeHash(const QByteArray& rawBytes) const
    {
        QImage img;
        img.loadFromData(rawBytes);
        return computeHash(img);
    }

    QString PerceptualHash::computeHash(const QImage& image) const
    {
        // 1. Resize to 32x32 and convert to grayscale
        QImage img = image.scaled(32, 32, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
            .convertToFormat(QImage::Format_Grayscale8);

        // 2. Copy pixels to a flat vector<double>
        std::vector<double> pixels;
        pixels.reserve(32 * 32);

        for (int y = 0; y < 32; ++y) {
            const uchar* line = img.scanLine(y);
            for (int x = 0; x < 32; ++x) {
                pixels.push_back(static_cast<double>(line[x]));
            }
        }

        // 3. Compute 2D DCT (32x32)
        std::vector<double> dct(32 * 32, 0.0);

        for (int u = 0; u < 32; ++u) {
            for (int v = 0; v < 32; ++v) {
                double sum = 0.0;
                for (int y = 0; y < 32; ++y) {
                    for (int x = 0; x < 32; ++x) {
                        sum += pixels[y * 32 + x] *
                            cos((2 * x + 1) * u * M_PI / 64) *
                            cos((2 * y + 1) * v * M_PI / 64);
                    }
                }
                double cu = (u == 0) ? 1.0 / sqrt(2.0) : 1.0;
                double cv = (v == 0) ? 1.0 / sqrt(2.0) : 1.0;
                dct[u * 32 + v] = 0.25 * cu * cv * sum;
            }
        }

        // 4. Keep top-left 8x8 DCT coefficients
        std::vector<double> dctLow;
        for (int i = 0; i < 8; ++i)
            for (int j = 0; j < 8; ++j)
                dctLow.push_back(dct[i * 32 + j]);

        // 5. Compute median
        std::vector<double> temp = dctLow;
        std::nth_element(temp.begin(), temp.begin() + temp.size() / 2, temp.end());
        double median = temp[temp.size() / 2];

        // 6. Generate 64-bit hash as hex string
        QString hash;
        for (double val : dctLow)
            hash += (val > median) ? "1" : "0";

        QString hexHash;
        for (int i = 0; i < hash.size(); i += 4)
            hexHash += QString::number(hash.mid(i, 4).toInt(nullptr, 2), 16);

        return hexHash;
    }

    double PerceptualHash::compareHash(const QString& hash1, const QString& hash2) const
    {
        if (hash1.size() != hash2.size())
            return -1; // or throw error

        int distance = 0;
        for (int i = 0; i < hash1.size(); ++i) {
            if (hash1[i] != hash2[i])
                ++distance;
        }
        return 1.0 - static_cast<double>(distance) / hash1.size();
    }
}