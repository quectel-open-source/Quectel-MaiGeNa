#pragma once
#include <array>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <vector>

template <typename T> class RingBuffer
{
  public:
    RingBuffer(int size, T initialValue = T()) : size_(size)
    {
        buffer_.resize(size_);
        std::fill(buffer_.begin(), buffer_.end(), initialValue);
    }

  public:
    bool canPush()
    {
        std::size_t write = write_.load();
        std::size_t next = (write + 1) % size_;
        return next != read_.load();
    }

    bool canPop()
    {
        return read_.load() != write_.load();
    }

    std::size_t size()
    {
        std::size_t read = read_.load();
        std::size_t write = write_.load();
        return write >= read ? write - read : size_ - (read - write);
    }

    std::size_t capacity()
    {
        return size_;
    }
    // empty
    bool empty()
    {
        return size() <= 0;
    }

    void clear()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        reset_ = true;
        condition_.notify_all();
        read_.store(0);
        write_.store(0);
        read_ = false;
    }

    // push with timeout， return false if timeout. if timeout is zero, it will not wait. timeout is ms. wait until push
    bool push(const T &item, std::chrono::milliseconds timeout = std::chrono::milliseconds::max())
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (condition_.wait_for(lock, timeout, [this] { return reset_.load() || canPush(); }))
        {
            buffer_[write_.load()] = item;
            write_.store((write_.load() + 1) % size_);
            return true;
        }

        return false;
    }

    // pop with timeout， return false if timeout. if timeout is zero, it will not wait. timeout is ms. wait until pop
    bool pop(T &item, std::chrono::milliseconds timeout = std::chrono::milliseconds::max())
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (condition_.wait_for(lock, timeout, [this] { return reset_.load() || canPop(); }))
        {
            if (empty())
                return false;
            item = std::move(buffer_[read_.load()]);
            read_.store((read_.load() + 1) % size_);
            return true;
        }

        return false;
    }

  private:
    const int size_;
    std::vector<T> buffer_;
    std::atomic<bool> reset_{false};
    std::atomic<std::size_t> read_{0};
    std::atomic<std::size_t> write_{0};
    std::condition_variable condition_;
    std::mutex mutex_;
};
