// Conway's Game of Life
// Copyright (c) 2025 Faraz Fallahi <fffaraz@gmail.com>

#pragma once

#include <mutex>
#include <shared_mutex>

#if 0 // Use a real mutex for the write buffer; not needed with a single writer.
using WriteMutex = std::mutex;
#else
class FakeMutex {
public:
    void lock() {}
    void unlock() {}
    bool try_lock() { return true; }
};
using WriteMutex = FakeMutex;
#endif

template <typename T>
class DoubleBuffer {
public:
    DoubleBuffer() = default;

    using read_lock = std::shared_lock<std::shared_mutex>;
    using write_lock = std::unique_lock<WriteMutex>;

    // Return a const reference to the current read buffer along with a shared read lock.
    std::pair<const T&, read_lock> readBuffer() const
    {
        read_lock lock(readMutex_);
        return { buffer_[index_], std::move(lock) };
    }

    // Return a reference to the current write buffer along with an exclusive write lock.
    std::pair<T&, write_lock> writeBuffer()
    {
        write_lock lock(writeMutex_);
        return { buffer_[1 - index_], std::move(lock) };
    }

    // Clone the current read buffer.
    T clone() const
    {
        read_lock lock(readMutex_);
        return buffer_[index_];
    }

    // Set new data in the write buffer and swap the buffers.
    void setAndSwap(T newData)
    {
        write_lock writeLock(writeMutex_);
        buffer_[1 - index_] = std::move(newData);
        swap(std::move(writeLock));
    }

    // Swap the read and write buffers.
    void swap()
    {
        write_lock writeLock(writeMutex_);
        swap(std::move(writeLock));
    }

    // Swap the read and write buffers using an existing write lock.
    void swap(write_lock&& writeLock)
    {
        std::unique_lock<std::shared_mutex> readLock(readMutex_); // Exclusive read lock to prevent reads during the swap.
        index_ = 1 - index_;
    }

private:
    T buffer_[2];
    int index_ = 0; // Read index (alternates between 0 and 1).
    WriteMutex writeMutex_;
    mutable std::shared_mutex readMutex_;
};
