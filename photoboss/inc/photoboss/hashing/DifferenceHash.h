#pragma once
#include "hashing/HashMethod.h"

namespace photoboss {

    class DifferenceHash : public HashMethod
    {
    public:
        QString compute(const PerceptualImage& image) override;
        double compare(const QString& hash1, const QString& hash2) const override;
        QString key() const override { return "Difference Hash"; }

        // Inherited via HashMethod
        HashInput InputType() const override;
    };
}