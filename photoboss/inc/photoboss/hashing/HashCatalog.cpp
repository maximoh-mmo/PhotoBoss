#include "HashCatalog.h"
#include "hashing/Sha256Hash.h"
#include "hashing/PerceptualHash.h"
#include "hashing/DifferenceHash.h"
#include "hashing/AverageHash.h"

namespace photoboss {

    std::vector<HashCatalog::Entry> HashCatalog::createAll()
    {
        std::vector<Entry> hashes;

        hashes.push_back({ "SHA256", std::make_unique<Sha256Hash>() });
        hashes.push_back({ "Perceptual Hash", std::make_unique<PerceptualHash>() });
        hashes.push_back({ "Difference Hash", std::make_unique<DifferenceHash>() });
        hashes.push_back({ "Average Hash", std::make_unique<AverageHash>() });

        return hashes;
    }

}