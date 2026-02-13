#pragma once
namespace photoboss::settings {
    // Thumbnails
    static inline constexpr int ThumbnailWidth = 140;
    static inline constexpr int ThumbnailsPerRow = 4;
    static inline constexpr int ThumbnailSpacing = 8;
    static inline constexpr int BadgeHeight = 20;
    static inline constexpr int MetaHeight = 40;

    // SQL Schema

    static inline constexpr int SCHEMA_VERSION = 0;
    // Hashing
    static inline constexpr int HashSampleSize = 32;

    // Scanning / batching
    static inline constexpr int DirectoryScanBatchSize = 200;

    // DiskReader
    static inline constexpr int DiskReaderProgressUpdateFrequency = 50;

}