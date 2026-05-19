#pragma once
#include <QMutex>
#include <QHash>
#include <QString>
#include <cstdio>
#include <limits>

namespace photoboss {

struct StageEntry {
    const char* name;
    qint64 minNs = std::numeric_limits<qint64>::max();
    qint64 maxNs = 0;
    quint64 totalNs = 0;
    quint64 count = 0;
};

class StageMetrics {
public:
    static StageMetrics& instance();

    void record(const char* name, qint64 elapsedNs);

    void printAll();

    void reset();

private:
    StageMetrics() = default;

    QMutex m_mutex;
    QHash<QString, StageEntry> m_entries;
};

} // namespace photoboss
