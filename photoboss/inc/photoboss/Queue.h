#pragma once
#include <deque>
#include <mutex>
#include <condition_variable>

template <typename T>
class Queue // A thread-safe queue implementation
{
    public:
    Queue(const Queue &) = delete;
    Queue(Queue &&) = delete;
    Queue &operator= (const Queue &) = delete;
    Queue &operator= (Queue &&) = delete;
    Queue() : b_is_bounded_(false), m_capacity_(-1) {}
    explicit Queue(size_t capacity) : b_is_bounded_ (capacity>0), m_capacity_(capacity) {}
    void shutdown();
private:
    bool b_shutdown_ = false;
    bool b_is_bounded_;
    std::deque<T> m_deque_;
    size_t m_capacity_;
    std::mutex m_mutex_;
    std::condition_variable m_not_empty_;
    std::condition_variable m_not_full_;

public:
    void push(T &&item) {
        std::unique_lock lock(m_mutex_);
        m_not_full_.wait(lock, [this]() { return m_deque_.size() < m_capacity_; });
        m_deque_.push_back(std::move(item));
        m_not_empty_.notify_one();
    }

    bool try_push(T &&item) {
        std::unique_lock lock(m_mutex_);
        if (m_deque_.size() == m_capacity_)
            return false;
        m_deque_.push_back(std::move(item));
        m_not_empty_.notify_one();
        return true;
    }

    void pop(T &item) {
        std::unique_lock lock(m_mutex_);
        m_not_empty_.wait(lock, [this]() { return !m_deque_.empty(); });
        item = std::move(m_deque_.front());
        m_deque_.pop_front();
        m_not_full_.notify_one();
    }

    bool try_pop(T &item) {
        std::unique_lock lock(m_mutex_);
        if (m_deque_.empty())
            return false;
        item = std::move(m_deque_.front());
        m_deque_.pop_front();
        m_not_full_.notify_one();
        return true;
    }

    void notify_all() {
        m_not_empty_.notify_all();
	}
};

template <typename T>
void Queue<T>::shutdown()
{
    std::unique_lock lock(m_mutex_);
    b_shutdown_ = true;
    m_not_empty_.notify_all();
    m_not_full_.notify_all();
}
