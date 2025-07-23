#pragma once
#include <qbytearray.h>
#include <QImage>

namespace photoboss
{
    class HashMethod
    {
    public:
        HashMethod() = delete; // Prevent instantiation
        virtual ~HashMethod() = default;
    
        virtual QString computeHash(const QByteArray& rawBytes) = 0;
        virtual QString computeHash(const QImage& image) = 0;
        virtual double compareHash(const QString& hash1, const QString& hash2) = 0;
        virtual QString getName() const = 0;
    };
} 