#pragma once
#include <string>

namespace photoboss {
    class PerceptualHash {
        public:
        PerceptualHash() = default;
        ~PerceptualHash() = default;
        // Generate a perceptual hash for the given image file
        static std::string generateAverageHash(const std::string& imagePath);
        static std::string generateDCTHash(const std::string& imagePath);
        // Compare two perceptual hashes and return a similarity score
        static double compareHashes(const std::string& hash1, const std::string& hash2);
    };
}
