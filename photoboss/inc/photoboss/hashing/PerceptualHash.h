#pragma once
#include "hashing/HashMethod.h"

namespace photoboss {

    class PerceptualHash : public HashMethod
    {
    public:
        QString compute(const QImage& image) override;
        double compare(const QString& hash1, const QString& hash2) const override;
        QString key() const override { return "Perceptual Hash"; }

        // Inherited via HashMethod
        HashInput InputType() const override;
    };
}