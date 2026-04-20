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

    // Delete Confirmation Dialog
    static inline constexpr int DeleteConfirmDialogMinWidth = 500;
    static inline constexpr int DeleteConfirmDialogMinHeight = 400;
    static inline constexpr int DeleteConfirmDialogThumbnailSize = 100;
    static inline constexpr int DeleteConfirmDialogGridCols = 4;
    static inline constexpr int DeleteConfirmDialogScrollAreaMinHeight = 200;
    static inline constexpr int DeleteConfirmDialogLayoutSpacing = 6;

    // UI Colors and Styles
    static inline constexpr char DeleteConfirmDialogWarningColor[] = "#ff6b6b";
    static inline constexpr char DeleteConfirmDialogBorderColor[] = "#555";
    static inline constexpr char DeleteConfirmDialogDeleteButtonBgColor[] = "#c0392b";
    static inline constexpr char DeleteConfirmDialogDeleteButtonTextColor[] = "white";
    static inline constexpr char DeleteConfirmDialogFailedLoadBorderColor[] = "#555";
    static inline constexpr char DeleteConfirmDialogFailedLoadTextColor[] = "#888";

    // Main Window
    static inline constexpr int MainWindowBatchProcessSize = 10;
    static inline constexpr int MainWindowBatchTimerInterval = 50; // ms
    static inline constexpr int MainWindowProgressTimerInterval = 30; // ms

}