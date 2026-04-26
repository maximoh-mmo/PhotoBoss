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

    // Storage-aware scanning
    static inline constexpr int SSDMaxThreads = 8;
    static inline constexpr int HDDBatchMultiplier = 4;

    // DiskReader
    static inline constexpr int DiskReaderProgressUpdateFrequency = 50;

    // Delete Confirmation Dialog
    static inline constexpr int DeleteConfirmDialogMinWidth = 500;
    static inline constexpr int DeleteConfirmDialogMinHeight = 400;
    static inline constexpr int DeleteConfirmDialogThumbnailSize = 100;
    static inline constexpr int DeleteConfirmDialogGridCols = 4;
    static inline constexpr int DeleteConfirmDialogScrollAreaMinHeight = 200;
    static inline constexpr int DeleteConfirmDialogLayoutSpacing = 6;

    // Main Window
    static inline constexpr int MainWindowBatchProcessSize = 10;
    static inline constexpr int MainWindowBatchTimerInterval = 50; // ms
    static inline constexpr int MainWindowProgressTimerInterval = 30; // ms

    // Progress throttling (time-based, per stage)
    static inline constexpr int ScannerProgressEmitIntervalMs = 200;   // 5/sec - Find phase
    static inline constexpr int HashingProgressEmitIntervalMs = 100;    // 10/sec - Analyze phase
    static inline constexpr int ResultProgressEmitIntervalMs = 100;    // 10/sec - Group phase

    // Similarity Engine
    static inline constexpr double SimilarityStrongThreshold = 0.97;
    static inline constexpr double SimilarityWeakThreshold = 0.92;
    static inline constexpr double SimilarityPHashGate = 0.98;
    static inline constexpr double SimilarityDHashGate = 0.94;

}