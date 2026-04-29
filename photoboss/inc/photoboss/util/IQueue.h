#pragma once
class IQueue {
public:
    virtual ~IQueue() = default;
    size_t id() const { return m_id_; }

    virtual void clear() = 0;
    virtual void request_shutdown(const photoboss::Token&) = 0;
protected:
    IQueue() : m_id_(next_id()) {}

private:
    size_t m_id_;

    static size_t next_id() {
        static std::atomic<size_t> counter{ 0 };
        return counter.fetch_add(1, std::memory_order_relaxed);
    }

    virtual void shutdown() = 0;
};
