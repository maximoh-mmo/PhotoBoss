#pragma once
#include <qnumeric.h>
#include <qstring.h>

namespace photoboss {
    static QString humanSize(qint64 bytes)
    {
        constexpr double KB = 1024.0;
        constexpr double MB = KB * 1024.0;
        constexpr double GB = MB * 1024.0;

        if (bytes >= GB) return QString::number(bytes / GB, 'f', 2) + " GB";
        if (bytes >= MB) return QString::number(bytes / MB, 'f', 2) + " MB";
        if (bytes >= KB) return QString::number(bytes / KB, 'f', 1) + " KB";
        return QString::number(bytes) + " B";
    }
}