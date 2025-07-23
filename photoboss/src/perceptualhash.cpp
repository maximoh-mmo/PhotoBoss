#include "perceptualhash.h"

#include <QString>

namespace photoboss
{
    QString PerceptualHash::computeHash(const QImage& image)
    {
        return "";
    }

    double PerceptualHash::compareHash(const QString& hash1, const QString& hash2)
    {
        return 0;
    }

    QString PerceptualHash::getName() const
    {
        return "Perceptual";
    }
    QString PerceptualHash::computeHash(const QByteArray& rawBytes)
    {
        return "";
    }
}
