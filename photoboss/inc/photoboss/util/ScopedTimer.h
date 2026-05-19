#pragma once
#include "util/StageMetrics.h"
#include <QElapsedTimer>

#ifdef QT_DEBUG

#define CONCAT2_(a, b) a##b
#define CONCAT_(a, b)  CONCAT2_(a, b)
#define SCOPED_TIMER(name) \
    photoboss::ScopedTimer CONCAT_(_st_, __LINE__)(name)

namespace photoboss {

class ScopedTimer {
public:
    explicit ScopedTimer(const char* name)
        : m_name(name)
    {
        m_timer.start();
    }

    ~ScopedTimer()
    {
        StageMetrics::instance().record(m_name, m_timer.nsecsElapsed());
    }

private:
    const char* m_name;
    QElapsedTimer m_timer;
};

} // namespace photoboss

#else

#define SCOPED_TIMER(name) ((void)0)

#endif
