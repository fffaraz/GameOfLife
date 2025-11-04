#pragma once

#include <mutex>
#include <shared_mutex>

template <typename T>
class DoubleBuffer {
public:
    DoubleBuffer() = default;

    using read_lock = std::shared_lock<std::shared_mutex>;
    using write_lock = std::unique_lock<std::shared_mutex>;

    std::pair<const T&, read_lock> readBuffer() const
    {
        read_lock lock(mutex_);
        return { buffer_[index_], std::move(lock) };
    }

    std::pair<T&, write_lock> writeBuffer()
    {
        write_lock lock(mutex_);
        return { buffer_[1 - index_], std::move(lock) };
    }

    std::tuple<const T&, T&, write_lock> buffers()
    {
        write_lock lock(mutex_);
        return { buffer_[index_], buffer_[1 - index_],  std::move(lock) };
    }

    void setAndSwap(T newData)
    {
        write_lock lock(mutex_);
        buffer_[1 - index_] = std::move(newData);
        index_ = 1 - index_;
    }

    void swap()
    {
        write_lock lock(mutex_);
        index_ = 1 - index_;
    }

private:
    T buffer_[2];
    int index_ = 0;
    mutable std::shared_mutex mutex_;
};
