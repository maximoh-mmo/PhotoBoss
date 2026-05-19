#include "util/StageMetrics.h"
#include <algorithm>
#include <cstring>

namespace photoboss {

StageMetrics& StageMetrics::instance()
{
    static StageMetrics metrics;
    return metrics;
}

void StageMetrics::record(const char* name, qint64 elapsedNs)
{
    QMutexLocker lock(&m_mutex);
    StageEntry& e = m_entries[QString::fromLatin1(name)];
    e.name = name;
    e.count++;
    e.totalNs += elapsedNs;
    if (elapsedNs < e.minNs) e.minNs = elapsedNs;
    if (elapsedNs > e.maxNs) e.maxNs = elapsedNs;
}

void StageMetrics::printAll()
{
    QMutexLocker lock(&m_mutex);
    fprintf(stderr, "\n--- Stage Timing Metrics ---\n");
    QList<StageEntry*> sorted;
    for (auto it = m_entries.begin(); it != m_entries.end(); ++it)
        sorted.append(&it.value());
    std::sort(sorted.begin(), sorted.end(),
        [](const StageEntry* a, const StageEntry* b) { return strcmp(a->name, b->name) < 0; });

    for (const auto* e : sorted) {
        double avgMs = (e->count > 0) ? (double)e->totalNs / e->count / 1e6 : 0.0;
        double minMs = e->minNs / 1e6;
        double maxMs = e->maxNs / 1e6;
        fprintf(stderr, "  %s: count=%llu, avg=%.3fms, min=%.3fms, max=%.3fms\n",
            e->name,
            (unsigned long long)e->count,
            avgMs, minMs, maxMs);
    }
    fprintf(stderr, "---\n\n");
}

void StageMetrics::reset()
{
    QMutexLocker lock(&m_mutex);
    m_entries.clear();
}

} // namespace photoboss
