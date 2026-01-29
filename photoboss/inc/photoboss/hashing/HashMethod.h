#pragma once

#include <QByteArray>
#include <QImage>
#include <QString>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <set>


namespace photoboss
{
    // Base class for hash / fingerprint algorithms
    class HashMethod
    {
    public:
        HashMethod() = default;
        virtual ~HashMethod() = default;

        virtual QString computeHash(const QImage& image) const = 0;

		// Compare two hashes, returning a similarity score in [0.0, 1.0]
		virtual double compareHash(const QString& hash1, const QString& hash2) const = 0;

        virtual QString key() const = 0;
    };

    // Concrete implementations forward declarations

    class PerceptualHash : public HashMethod
    {
    public:
        QString computeHash(const QImage& image) const override;
        double compareHash(const QString& hash1, const QString& hash2) const override;
		QString key() const override { return "Perceptual Hash"; }
    };
}