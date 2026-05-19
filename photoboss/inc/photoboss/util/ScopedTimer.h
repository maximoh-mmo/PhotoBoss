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
        : m_name_(name)
    {
        m_timer_.start();
    }

    ~ScopedTimer()
    {
        StageMetrics::instance().record(m_name_, m_timer_.nsecsElapsed());
    }

private:
    const char* m_name_;
    QElapsedTimer m_timer_;
};

} // namespace photoboss

#else

#define SCOPED_TIMER(name) ((void)0)

#endif
