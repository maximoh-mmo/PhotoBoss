#include "hashing/Sha256Hash.h"
#include <qcryptographichash.h>
#include "hashing/HashRegistry.h"

namespace photoboss
{
    const bool ph_registered =
        photoboss::HashRegistry::registerFactory<photoboss::Sha256Hash>("SHA256");

    double Sha256Hash::compare(const QString& hash1, const QString& hash2) const
    {
        return (hash1 == hash2) ? 1.0 : 0.0;
    }

    QString Sha256Hash::computeSHA256(const QByteArray& data)
    {
        QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha256);
        return hash.toHex();  // Converts the raw bytes to a hex string
    }
}