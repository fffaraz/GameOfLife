// Conway's Game of Life
// Copyright (c) 2025 Faraz Fallahi <fffaraz@gmail.com>

#pragma once

#include <atomic>
#include <barrier>
#include <thread>
#include <type_traits>
#include <vector>

// Persistent worker pool: threads stay alive across generations, and each owns a
// fixed band of work. Two reusable barriers gate the start and end of each pass,
// so we no longer pay a parallel-for's task dispatch (fork/join) every single
// generation. Only one coordinator thread may call run() at a time.
class BandExecutor {
public:
    explicit BandExecutor(int numThreads)
        : numThreads_(numThreads > 0 ? numThreads : 1)
        , start_(numThreads_)
        , done_(numThreads_)
    {
        // Band 0 is run by the calling (coordinator) thread; spawn workers for the rest.
        for (int t = 1; t < numThreads_; ++t) {
            workers_.emplace_back([this, t] {
                while (true) {
                    start_.arrive_and_wait();
                    if (stop_.load(std::memory_order_acquire)) {
                        return;
                    }
                    thunk_(ctx_, t);
                    done_.arrive_and_wait();
                }
            });
        }
    }

    ~BandExecutor()
    {
        if (numThreads_ > 1) {
            stop_.store(true, std::memory_order_release);
            start_.arrive_and_wait(); // wake parked workers so they observe stop_ and exit
        }
        // workers_ (jthreads) join in their destructors.
    }

    BandExecutor(const BandExecutor&) = delete;
    BandExecutor& operator=(const BandExecutor&) = delete;

    int size() const { return numThreads_; }

    // Invoke fn(bandIndex) on every band 0..size()-1 and block until all finish.
    // fn stays alive for the whole call, so it is type-erased without allocating.
    template <typename F>
    void run(F&& fn)
    {
        ctx_ = &fn;
        thunk_ = [](void* p, int t) { (*static_cast<std::remove_reference_t<F>*>(p))(t); };
        if (numThreads_ == 1) {
            thunk_(ctx_, 0);
            return;
        }
        start_.arrive_and_wait(); // release workers; they now observe ctx_/thunk_
        thunk_(ctx_, 0); // coordinator runs band 0
        done_.arrive_and_wait(); // wait for all workers to finish this pass
    }

private:
    const int numThreads_;
    std::atomic<bool> stop_ { false };
    void* ctx_ = nullptr;
    void (*thunk_)(void*, int) = nullptr;
    std::barrier<> start_;
    std::barrier<> done_;
    std::vector<std::jthread> workers_;
};
