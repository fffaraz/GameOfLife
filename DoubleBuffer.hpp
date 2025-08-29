#pragma once

#include <mutex>

template <typename T>
class DoubleBuffer {
public:
    DoubleBuffer()
        : front_ { buffer[0] }
        , back_ { buffer[1] }
    {
    }

    std::pair<T&, T&> get()
    {
        mutex_.lock();
        return { front_, back_ };
    }

    void unlock() { mutex_.unlock(); }

    void swap()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::swap(front_, back_);
    }

private:
    T buffer[2];
    T& front_;
    T& back_;
    std::mutex mutex_;
};
