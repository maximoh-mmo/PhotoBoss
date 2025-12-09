#include "HashMethod.h"
#include <QCryptographicHash>
#include <QBuffer>
namespace photoboss
{
    const bool md5_registered =
        photoboss::HashRegistry::registerFactory<photoboss::Md5>(QStringLiteral("Md5"));

    QString Md5::computeHash(const QByteArray& rawBytes) const
    {
        QCryptographicHash hasher(QCryptographicHash::Md5);
        hasher.addData(rawBytes);
        QString hash = hasher.result().toHex();
        return hasher.result().toHex();
    }

    QString Md5::computeHash(const QImage& image) const
    {
        QByteArray arr;
        QBuffer buffer(&arr);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer);
        return computeHash(arr);
    }

    double Md5::compareHash(const QString& hash1, const QString& hash2) const
    {
		return hash1 == hash2 ? 1.0 : 0.0;
    }
}