#pragma once

#include "util/DataTypes.h"

namespace photoboss
{
    // Base class for hash / fileIdentity algorithms
    class HashMethod
    {
    public:
        virtual ~HashMethod() = default;

        virtual QString key() const = 0;
        virtual HashInput InputType() const = 0;

        virtual QString compute(const QByteArray&) {
            throw std::logic_error("Byte input not supported");
        }

        virtual QString compute(const QImage&) {
            throw std::logic_error("Image input not supported");
        }

		// Compare two hashes, returning a similarity score in [0.0, 1.0]
		virtual double compare(const QString& hash1, const QString& hash2) const = 0;

    };
}