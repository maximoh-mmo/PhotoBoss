#pragma once
#include "hashing/HashMethod.h"
#include "util/DataTypes.h"
namespace photoboss {
    class Sha256Hash : public HashMethod
    {
    public:
        HashInput InputType() const override { return HashInput::Bytes; }
        QString compute(const QByteArray& data) { return computeSHA256(data); }
        QString key() const override { return "SHA256"; }
        double compare(const QString& hash1, const QString& hash2) const override;
    private:
        QString computeSHA256(const QByteArray& data);
    };
}

