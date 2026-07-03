// Conway's Game of Life
// Copyright (c) 2025 Faraz Fallahi <fffaraz@gmail.com>

#pragma once

#include <array>
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

template <int SIZE>
class alignas(64) Grid {
public:
    inline bool get(const Point& p) const { return grid_[index(p)]; }
    inline void set(const Point& p, const bool value) { grid_[index(p)] = value; }
    inline void toggle(const Point& p)
    {
        const int idx = index(p);
        grid_[idx] = !grid_[idx];
    }
    int countLiveNeighbors(const Point& p) const;
    void toggleBlock(const Point& p);
    void updateGrid(const Grid<SIZE>& current);
    void updateNeighbors();
    void addNoise(int n = 1);
    void clear();

private:
    std::array<bool, SIZE * SIZE> grid_;
    inline void updateRow(const Grid<SIZE>& current, int x);
    inline static int index(const Point& p) { return (p.x * SIZE) + p.y; }
};

// Count the number of live neighbors for the cell at (x, y)
template <int SIZE>
int Grid<SIZE>::countLiveNeighbors(const Point& p) const
{
    // Fast path for interior cells: no bounds checks and no per-neighbor index math
    if (p.x > 0 && p.x < SIZE - 1 && p.y > 0 && p.y < SIZE - 1) {
        const int idx = index(p);
        // clang-format off
        return grid_[idx - SIZE - 1] // Top-left
             + grid_[idx - SIZE    ] // Top
             + grid_[idx - SIZE + 1] // Top-right
             + grid_[idx        - 1] // Left
             + grid_[idx        + 1] // Right
             + grid_[idx + SIZE - 1] // Bottom-left
             + grid_[idx + SIZE    ] // Bottom
             + grid_[idx + SIZE + 1] // Bottom-right
             ;
        // clang-format on
    }
    // General case with bounds checks
    int liveNeighbors = 0;
    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            if (i == 0 && j == 0)
                continue; // Skip the cell itself
            const int nx = p.x + i; // (p.x + i) % SIZE;
            const int ny = p.y + j; // (p.y + j) % SIZE;
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

// Apply the rules of Conway's Game of Life
static inline bool gameOfLife(const bool cell, const int liveNeighbors)
{
    // A cell is alive in the next generation if it has 3 neighbors,
    // or if it is currently alive and has 2 neighbors.
    return liveNeighbors == 3 || (cell && liveNeighbors == 2);
}

// Update a single row. Border rows/columns use the general path; the interior
// slides a 3-wide window of column sums so each cell reads only one fresh
// column (3 loads) instead of all 8 neighbors, with no per-cell branch.
template <int SIZE>
inline void Grid<SIZE>::updateRow(const Grid<SIZE>& current, const int x)
{
    // Top and bottom edge rows have no full 3x3 neighborhood: use the general path.
    if (x == 0 || x == SIZE - 1) {
        for (int y = 0; y < SIZE; ++y) {
            const Point p { x, y };
            const int idx = index(p);
            grid_[idx] = gameOfLife(current.grid_[idx], current.countLiveNeighbors(p));
        }
        return;
    }

    // Distinct grids (double buffer / ping-pong), so out never aliases the inputs.
    const bool* const __restrict top = &current.grid_[(x - 1) * SIZE];
    const bool* const __restrict mid = &current.grid_[x * SIZE];
    const bool* const __restrict bot = &current.grid_[(x + 1) * SIZE];
    bool* const __restrict out = &grid_[x * SIZE];

    // Left/right edge columns have no full neighborhood: general path.
    out[0] = gameOfLife(mid[0], current.countLiveNeighbors({ x, 0 }));
    out[SIZE - 1] = gameOfLife(mid[SIZE - 1], current.countLiveNeighbors({ x, SIZE - 1 }));

    // Interior: liveNeighbors = colSum(y-1) + colSum(y) + colSum(y+1) - self.
    int colPrev = top[0] + mid[0] + bot[0];
    int colCurr = top[1] + mid[1] + bot[1];
    for (int y = 1; y < SIZE - 1; ++y) {
        const int colNext = top[y + 1] + mid[y + 1] + bot[y + 1];
        const int live = colPrev + colCurr + colNext - mid[y];
        out[y] = gameOfLife(mid[y], live);
        colPrev = colCurr;
        colCurr = colNext;
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
    grid_.fill(false);
}
