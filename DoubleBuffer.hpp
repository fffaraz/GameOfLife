#pragma once

#include <mutex>
#include <shared_mutex>

template <typename T>
class DoubleBuffer {
public:
    DoubleBuffer() = default;

    using read_lock = std::shared_lock<std::shared_mutex>;
    using write_lock = std::unique_lock<std::mutex>;

    // Returns a const reference to the current read buffer along with a read lock
    std::pair<const T&, read_lock> readBuffer() const
    {
        read_lock lock(readMutex_);
        return { buffer_[index_], std::move(lock) };
    }

    // Returns a reference to the current write buffer along with a write lock
    std::pair<T&, write_lock> writeBuffer()
    {
        write_lock lock(writeMutex_);
        return { buffer_[1 - index_], std::move(lock) };
    }

    // Sets new data to the write buffer and swaps the buffers
    void setAndSwap(T newData)
    {
        write_lock writeLock(writeMutex_);
        buffer_[1 - index_] = std::move(newData);
        swap(std::move(writeLock));
    }

    // Swaps the read and write buffers
    void swap()
    {
        write_lock writeLock(writeMutex_);
        swap(std::move(writeLock));
    }

    void swap(write_lock&& writeLock)
    {
        std::unique_lock readLock(readMutex_); // exclusive lock to prevent reads during swap
        index_ = 1 - index_;
    }

private:
    T buffer_[2];
    int index_ = 0; // read index (alternates between 0 and 1)
    mutable std::mutex writeMutex_;
    mutable std::shared_mutex readMutex_;
};
