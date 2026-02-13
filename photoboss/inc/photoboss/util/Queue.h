#pragma once
#include <deque>
#include <mutex>
#include <condition_variable>
#include "util/Token.h"

/// <summary>
/// A Thread-safe queue supporting bounded and unbounded modes, with proper shutdown handling.
/// 
/// NOTE:
/// Queue shutdown is the ONLY supported stop mechanism.
/// Consumers and producers must exit when wait_and_pop or push return false.
/// </summary>
/// <typeparam name="T"></typeparam>
template <typename T>
class Queue
{
public:
    Queue(const Queue&) = delete;
    Queue(Queue&&) = delete;
    Queue& operator= (const Queue&) = delete;
    Queue& operator= (Queue&&) = delete;

    // Unbounded queue
    Queue() : m_capacity_(std::numeric_limits<size_t>::max()) {}

    // Bounded queue with specified capacity
    explicit Queue(size_t capacity) : m_capacity_(capacity) {}

    // Push item into the queue, returns false if shutdown occurred.
    bool push(T&& item) {
        std::unique_lock lock(m_mutex_);
        if (!wait_for_space(lock)) return false;
        m_deque_.push_back(std::move(item));
        m_not_empty_.notify_one();
        return true;
    }
    // Emplace an item in-place; returns false if shutdown occurred
    template <typename... Args>
    bool emplace(Args&&... args) {
        std::unique_lock lock(m_mutex_);
        if (!wait_for_space(lock)) return false;
        m_deque_.emplace_back(std::forward<Args>(args)...);
        m_not_empty_.notify_one();
        return true;
    }

    // Try to push without blocking; returns false if full or shutdown
    bool try_push(T&& item) {
        std::unique_lock lock(m_mutex_);
        if (m_deque_.size() >= m_capacity_ || b_shutdown_)
            return false;
        m_deque_.push_back(std::move(item));
        m_not_empty_.notify_one();
        return true;
    }

    // Pop an item, blocks until available or shutdown. Returns false if queue empty & shutdown.
    bool wait_and_pop(T& item) {
        std::unique_lock lock(m_mutex_);
        m_not_empty_.wait(lock, [this]() { return !m_deque_.empty() || b_shutdown_; });
        if (b_shutdown_ && m_deque_.empty()) return false;
        item = std::move(m_deque_.front());
        m_deque_.pop_front();
        m_not_full_.notify_one();
        return true;
    }

    // Try pop without blocking, returns false if empty
    bool try_pop(T& item) {
        std::unique_lock lock(m_mutex_);
        if (m_deque_.empty())
            return false;
        item = std::move(m_deque_.front());
        m_deque_.pop_front();
        m_not_full_.notify_one();
        return true;
    }

    // Clears the queue and notifies waiting threads
    void clear() {
        std::unique_lock lock(m_mutex_);
        m_deque_.clear();
        m_not_empty_.notify_all();
        m_not_full_.notify_all();
    }

    // Notify all waiting threads (producers and consumers)
    void notify_all() {
        m_not_empty_.notify_all();
        m_not_full_.notify_all();
    }

    // Check if queue is empty
    bool empty() const {
        std::unique_lock lock(m_mutex_);
        return m_deque_.empty();
    }

    // Current queue size
    size_t size() const {
        std::unique_lock lock(m_mutex_);
        return m_deque_.size();
    }

    // Reset shutdown flag (use with caution)
    void reset_shutdown() {
        std::unique_lock lock(m_mutex_);
        b_shutdown_ = false;
    }

    // Register Producer that uses this queue
    void register_producer() {
        m_producers_.fetch_add(1, std::memory_order_relaxed);
    }

    // Signal Producer is finished, when no more producers the queue will shut down.
    void producer_done() {
        const auto remaining =
            m_producers_.fetch_sub(1, std::memory_order_acq_rel) - 1;

        if (remaining == 0) {
            shutdown();
        }
    }

    // Request Shutdown
    void request_shutdown(const photoboss::Token&) {
        shutdown();
    }

private:
    bool b_shutdown_ = false;
    std::deque<T> m_deque_;
    const size_t m_capacity_;
    mutable std::mutex m_mutex_;
    std::atomic<size_t> m_producers_{ 0 };
    std::condition_variable m_not_empty_;
    std::condition_variable m_not_full_;

    // Signal Shutdown and wake all waiting threads
    void shutdown() {
        std::unique_lock lock(m_mutex_);
        b_shutdown_ = true;
        m_not_empty_.notify_all();
        m_not_full_.notify_all();
    }

    // Wait until there is space in the queue (for bounded) or shutdown
    bool wait_for_space(std::unique_lock<std::mutex>& lock) {
        m_not_full_.wait(lock, [this]() { return m_deque_.size() < m_capacity_ || b_shutdown_; });
        return !b_shutdown_;
    }
};