#pragma once
#include "hashing/HashMethod.h"
#include <memory>
#include <vector>

namespace photoboss {

    class HashCatalog final
    {
    public:
        struct Entry {
            QString key;
            std::unique_ptr<HashMethod> method;
        };

        // Create a fresh set of all hash methods.
        // Each caller owns its own instances (thread-safe by design).
        static std::vector<Entry> createAll();
    };

}