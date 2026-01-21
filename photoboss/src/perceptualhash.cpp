#include <QString>
#include <QtMath>
#include <vector>
#include <algorithm>
#include "HashMethod.h"

namespace photoboss
{
    // Linker trick to ensure registration
    const bool ph_registered =
        photoboss::HashRegistry::registerFactory<photoboss::PerceptualHash>("Perceptual Hash");

    struct DCTConstants {
        double matrix[32][32];

        DCTConstants() {
            for (int i = 0; i < 32; ++i) {
                // The DCT-II basis formula
                double alpha = (i == 0) ? std::sqrt(1.0 / 32.0) : std::sqrt(2.0 / 32.0);
                for (int j = 0; j < 32; ++j) {
                    matrix[i][j] = alpha * std::cos((M_PI * i * (2.0 * j + 1.0)) / 64.0);
                }
            }
        }
    };

    QString PerceptualHash::computeHash(const QImage& image) const
    {
        static const DCTConstants dct_consts;

        // 1. Resize and Grayscale
        QImage img = image.scaled(32, 32, Qt::IgnoreAspectRatio, Qt::FastTransformation)
            .convertToFormat(QImage::Format_Grayscale8);

        // 2. Load pixels into double array
        double pixels[32][32];
        for (int y = 0; y < 32; ++y) {
            const uchar* line = img.scanLine(y);
            for (int x = 0; x < 32; ++x) {
                pixels[y][x] = static_cast<double>(line[x]);
            }
        }

        // 3. Compute Partial 2D DCT (Only the 8x8 top-left area)
        // Operation: Temp = Matrix * Pixels
        double temp[8][32];
        for (int i = 0; i < 8; ++i) {
            for (int j = 0; j < 32; ++j) {
                double sum = 0.0;
                for (int k = 0; k < 32; ++k) {
                    sum += dct_consts.matrix[i][k] * pixels[k][j];
                }
                temp[i][j] = sum;
            }
        }

        // Operation: Result = Temp * Matrix_Transposed
        double dct_final[8][8];
        for (int i = 0; i < 8; ++i) {
            for (int j = 0; j < 8; ++j) {
                double sum = 0.0;
                for (int k = 0; k < 32; ++k) {
                    sum += temp[i][k] * dct_consts.matrix[j][k];
                }
                dct_final[i][j] = sum;
            }
        }

        // 4. Flatten to 64 values and find median
        // Note: We skip dct_final[0][0] (the DC coefficient) for better frequency analysis
        std::vector<double> flat;
        flat.reserve(64);
        for (int i = 0; i < 8; ++i) {
            for (int j = 0; j < 8; ++j) {
                flat.push_back(dct_final[i][j]);
            }
        }


        std::vector<double> sorted_copy = flat;
        // Use 1 to skip the first element (DC coefficient) for the median
        auto median_it = sorted_copy.begin() + 32;
        std::nth_element(sorted_copy.begin() + 1, median_it, sorted_copy.end());
        double median = *median_it;

        // 5. Build 64-bit integer hash
        quint64 hash_val = 0;
        for (int i = 0; i < 64; ++i) {
            if (flat[i] > median) {
                hash_val |= (1ULL << i);
            }
        }

        // 6. Return as 16-character hex string
        return QString("%1").arg(hash_val, 16, 16, QChar('0'));
    }

    double PerceptualHash::compareHash(const QString& hash1, const QString& hash2) const
    {
        if (hash1.length() != hash2.length() || hash1.isEmpty()) return 0.0;

        // Convert hex strings back to bitsets (uint64)
        bool ok1, ok2;
        quint64 v1 = hash1.toULongLong(&ok1, 16);
        quint64 v2 = hash2.toULongLong(&ok2, 16);

        if (!ok1 || !ok2) return 0.0;

        // XOR finds differing bits, qPopulationCount counts them
        int distance = qPopulationCount(v1 ^ v2);

        // Return similarity (1.0 = identical, 0.0 = completely different)
        return 1.0 - (static_cast<double>(distance) / 64.0);
    }
}