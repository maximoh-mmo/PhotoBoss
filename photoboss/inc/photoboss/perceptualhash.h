#pragma once

#include "HashMethod.h"

namespace photoboss {
    class PerceptualHash : public HashMethod
    {
    public:
        PerceptualHash();
        double compareHash(const QString& hash1, const QString& hash2) override;
        QString getName() const override;
        QString computeHash(const QByteArray& rawBytes) override;
        QString computeHash(const QImage& image) override;
    };
}
