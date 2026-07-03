// Conway's Game of Life
// Copyright (c) 2025 Faraz Fallahi <fffaraz@gmail.com>

#pragma once

#include <array>
#include <cstdint>
#include <random>

#if 1 // Enable multithreaded calculation of grid updates
#define PARALLEL_GRID 1
#include "BandExecutor.hpp"

// Worker count: GOL_THREADS env override if set, else half the hardware threads.
static int defaultGridThreads()
{
    if (const char* env = std::getenv("GOL_THREADS")) {
        const int n = std::atoi(env);
        if (n > 0) {
            return n;
        }
    }
    return static_cast<int>(std::max(1u, std::thread::hardware_concurrency() / 2));
}

static BandExecutor gridExecutor { defaultGridThreads() };
#endif

struct Point {
    const int x;
    const int y;
};

// Bit-packed grid: one bit per cell, 64 cells per 64-bit word. Cell (x, y) lives
// in word row x, at bit (y & 63) of word (y >> 6). Packing the grid 8x tighter
// than one byte/cell both shrinks the working set (more of it stays in cache) and
// lets updateGrid evaluate 64 cells at once with SWAR bitwise arithmetic.
template <int SIZE>
class alignas(64) Grid {
    static_assert(SIZE % 64 == 0, "bit-packed Grid requires SIZE to be a multiple of 64");

public:
    inline bool get(const Point& p) const
    {
        return (words_[wordIndex(p)] >> bitOffset(p)) & 1ULL;
    }
    inline void set(const Point& p, const bool value)
    {
        const uint64_t mask = 1ULL << bitOffset(p);
        uint64_t& w = words_[wordIndex(p)];
        w = value ? (w | mask) : (w & ~mask);
    }
    inline void toggle(const Point& p)
    {
        words_[wordIndex(p)] ^= 1ULL << bitOffset(p);
    }
    int countLiveNeighbors(const Point& p) const;
    void toggleBlock(const Point& p);
    void updateGrid(const Grid<SIZE>& current);
    void addNoise(int n = 1);
    void clear();

private:
    static constexpr int WORDS_PER_ROW = SIZE / 64;
    alignas(64) std::array<uint64_t, SIZE * WORDS_PER_ROW> words_ {};

    inline void updateRow(const Grid<SIZE>& current, int x);
    inline static int wordIndex(const Point& p) { return (p.x * WORDS_PER_ROW) + (p.y >> 6); }
    inline static int bitOffset(const Point& p) { return p.y & 63; }
};

// Count the number of live neighbors for the cell at (x, y). Used only for
// rendering (cell coloring), so bit extraction with bounds checks is plenty.
template <int SIZE>
int Grid<SIZE>::countLiveNeighbors(const Point& p) const
{
    int liveNeighbors = 0;
    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            if (i == 0 && j == 0)
                continue; // Skip the cell itself
            const int nx = p.x + i;
            const int ny = p.y + j;
            if (nx >= 0 && nx < SIZE && ny >= 0 && ny < SIZE) {
                liveNeighbors += get({ nx, ny }) ? 1 : 0;
            }
        }
    }
    return liveNeighbors;
}

// Toggle a 3x3 block of cells at the given position
template <int SIZE>
void Grid<SIZE>::toggleBlock(const Point& p)
{
    const int size = 1;
    for (int i = -size; i <= size; ++i) {
        for (int j = -size; j <= size; ++j) {
            const Point n { p.x + i, p.y + j };
            if (n.x >= 0 && n.x < SIZE && n.y >= 0 && n.y < SIZE) {
                toggle(n);
            }
        }
    }
}

// Update one row using SWAR: 64 cells per word are evaluated with pure bitwise
// arithmetic. For the three source rows we build, per column, the sum of its 8
// neighbors via full/half adders on bit-planes, keeping the low 3 bits s0,s1,s2
// of that 0..8 count. Conway's rule then collapses to a single expression:
//   next = s1 & ~s2 & (s0 | self)
// i.e. alive next iff neighbor count is 2 (and self alive) or exactly 3.
// Grid edges are non-toroidal: zeros shift in past the first/last word and past
// the top/bottom rows, so out-of-bounds neighbors read as dead automatically.
template <int SIZE>
inline void Grid<SIZE>::updateRow(const Grid<SIZE>& current, const int x)
{
    constexpr int WPR = WORDS_PER_ROW;
    const uint64_t* const __restrict cur = current.words_.data();
    uint64_t* const __restrict out = words_.data();

    const int midBase = x * WPR;
    const bool hasTop = x > 0;
    const bool hasBot = x < SIZE - 1;
    const int topBase = midBase - WPR;
    const int botBase = midBase + WPR;

    for (int w = 0; w < WPR; ++w) {
        // Center words for the three rows (0 past the top/bottom edges).
        const uint64_t aC = hasTop ? cur[topBase + w] : 0;
        const uint64_t bC = cur[midBase + w];
        const uint64_t cC = hasBot ? cur[botBase + w] : 0;

        // Adjacent words feed the bit that crosses a 64-cell boundary; 0 at the
        // left/right grid edge so past-the-edge columns read as dead.
        const bool hasPrev = w > 0;
        const bool hasNext = w < WPR - 1;
        const uint64_t aP = (hasTop && hasPrev) ? cur[topBase + w - 1] : 0;
        const uint64_t aN = (hasTop && hasNext) ? cur[topBase + w + 1] : 0;
        const uint64_t bP = hasPrev ? cur[midBase + w - 1] : 0;
        const uint64_t bN = hasNext ? cur[midBase + w + 1] : 0;
        const uint64_t cP = (hasBot && hasPrev) ? cur[botBase + w - 1] : 0;
        const uint64_t cN = (hasBot && hasNext) ? cur[botBase + w + 1] : 0;

        // Left/right neighbor bit-planes for each row.
        const uint64_t aL = (aC << 1) | (aP >> 63);
        const uint64_t aR = (aC >> 1) | (aN << 63);
        const uint64_t bL = (bC << 1) | (bP >> 63);
        const uint64_t bR = (bC >> 1) | (bN << 63);
        const uint64_t cL = (cC << 1) | (cP >> 63);
        const uint64_t cR = (cC >> 1) | (cN << 63);

        // Top row: sum of its 3 columns -> 2-bit value (t1 t0).
        const uint64_t t0 = aL ^ aC ^ aR;
        const uint64_t t1 = (aL & aC) | (aC & aR) | (aL & aR);
        // Bottom row: sum of its 3 columns -> (u1 u0).
        const uint64_t u0 = cL ^ cC ^ cR;
        const uint64_t u1 = (cL & cC) | (cC & cR) | (cL & cR);
        // Middle row: left + right only (self excluded) -> (v1 v0).
        const uint64_t v0 = bL ^ bR;
        const uint64_t v1 = bL & bR;

        // Add the three 2-bit numbers; keep the low 3 bits of the 0..8 total.
        const uint64_t s0 = t0 ^ u0 ^ v0;
        const uint64_t c0 = (t0 & u0) | (u0 & v0) | (t0 & v0);
        const uint64_t hs = t1 ^ u1 ^ v1;
        const uint64_t hc = (t1 & u1) | (u1 & v1) | (t1 & v1);
        const uint64_t s1 = hs ^ c0;
        const uint64_t s2 = hc ^ (hs & c0);

        out[midBase + w] = s1 & ~s2 & (s0 | bC);
    }
}

#ifndef PARALLEL_GRID

// Update the grid based on the rules of Conway's Game of Life
template <int SIZE>
void Grid<SIZE>::updateGrid(const Grid<SIZE>& current)
{
    for (int x = 0; x < SIZE; ++x) {
        updateRow(current, x);
    }
}

#else // PARALLEL_GRID

// Parallel version of the update function: each band owns a contiguous, fixed
// range of rows every generation, keeping its slice warm in that core's cache.
template <int SIZE>
void Grid<SIZE>::updateGrid(const Grid<SIZE>& current)
{
    const int n = gridExecutor.size();
    gridExecutor.run([this, &current, n](int t) {
        const int begin = static_cast<int>(static_cast<long long>(t) * SIZE / n);
        const int end = static_cast<int>(static_cast<long long>(t + 1) * SIZE / n);
        for (int x = begin; x < end; ++x) {
            updateRow(current, x);
        }
    });
}

#endif

static std::mt19937 generator(0);
static std::uniform_int_distribution<int> distribution(0, 2'000'000'000);

// Add random noise to the grid
template <int SIZE>
void Grid<SIZE>::addNoise(int n)
{
    for (int i = 0; i < n; ++i) {
        const int x = distribution(generator) % SIZE;
        const int y = distribution(generator) % SIZE;
        toggle({ x, y });
    }
}

// Clear the grid
template <int SIZE>
void Grid<SIZE>::clear()
{
    words_.fill(0ULL);
}
