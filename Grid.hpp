#pragma once

#include <array>
#include <cstdlib>

#if 1
#include <execution>
#include <numeric>
#include <poolstl/poolstl.hpp>
#define PARALLEL_GRID 1
static task_thread_pool::task_thread_pool threadPool{std::thread::hardware_concurrency() / 2};
#endif

struct Point {
    const int x;
    const int y;
};

template <int SIZE>
class Grid {
public:
    int countLiveNeighbours(const Point& p) const;
    inline bool get(const Point& p) const { return grid_[index(p)]; }
    inline void set(const Point& p, const bool value) { grid_[index(p)] = value; }
    inline void toggle(const Point& p) { const int idx = index(p); grid_[idx] = !grid_[idx]; }
    // inline int neighbors(const Point& p) const { return neighbors_[index(p)]; }
    void toggleBlock(const Point& p);
    void update(const Grid<SIZE>& current);
    void addNoise(int n = 1);
    void clear();

private:
    std::array<bool, SIZE * SIZE> grid_;
    // std::array<uint8_t, SIZE * SIZE> neighbors_;
    inline static int index(const Point& p) { return (p.x * SIZE) + p.y; }
#ifdef PARALLEL_GRID
    static constexpr std::array<int, SIZE> indices = [](){ std::array<int, SIZE> v; std::iota(v.begin(), v.end(), 0); return v; }();
#endif
};

// Function to count the number of live neighbours for a cell at (x, y)
template <int SIZE>
int Grid<SIZE>::countLiveNeighbours(const Point& p) const
{
    // Fast path for interior cells: no bounds checks and no per-neighbor index math
    if (p.x > 0 && p.x < SIZE - 1 && p.y > 0 && p.y < SIZE - 1) {
        const int idx = index(p);
        return grid_[idx - SIZE - 1]  // Top-left
             + grid_[idx - SIZE    ]  // Top
             + grid_[idx - SIZE + 1]  // Top-right
             + grid_[idx        - 1]  // Left
             + grid_[idx        + 1]  // Right
             + grid_[idx + SIZE - 1]  // Bottom-left
             + grid_[idx + SIZE    ]  // Bottom
             + grid_[idx + SIZE + 1]; // Bottom-right
    }
    // General case with bounds checks
    int liveNeighbours = 0;
    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            if (i == 0 && j == 0)
                continue; // Skip the cell itself
            const int nx = p.x + i;
            const int ny = p.y + j;
            if (nx >= 0 && nx < SIZE && ny >= 0 && ny < SIZE) {
                liveNeighbours += get({ nx, ny }) ? 1 : 0;
            }
        }
    }
    return liveNeighbours;
}

// Function to toggle a 3x3 block of cells at the given position
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

// Function to apply the rules of Conway's Game of Life
static inline bool gameOfLife(const bool cell, const int liveNeighbours)
{
    // A cell is alive in the next generation if it has 3 neighbours,
    // or if it is currently alive and has 2 neighbours.
    return liveNeighbours == 3 || (cell && liveNeighbours == 2);
}

#ifndef PARALLEL_GRID

// Function to update the grid based on the rules of Conway's Game of Life
template <int SIZE>
void Grid<SIZE>::update(const Grid<SIZE>& current)
{
    for (int x = 0; x < SIZE; ++x) {
        for (int y = 0; y < SIZE; ++y) {
            const Point p { x, y };
            const int idx = index(p);
            const int live = current.countLiveNeighbours(p);
            grid_[idx] = gameOfLife(current.grid_[idx], live);
            // neighbors_[idx] = live;
        }
    }
}

#else // PARALLEL_GRID

// Parallel version of the update function
template <int SIZE>
void Grid<SIZE>::update(const Grid<SIZE>& current)
{
    // std::execution::par_unseq
    std::for_each(poolstl::par.on(threadPool), indices.begin(), indices.end(),
        [&](int x) {
            for (int y = 0; y < SIZE; ++y) {
                const Point p { x, y };
                const int idx = index(p);
                const int live = current.countLiveNeighbours(p);
                grid_[idx] = gameOfLife(current.grid_[idx], live);
                // neighbors_[idx] = live;
            }
        });
}

#endif

// Function to add random noise to the grid
template <int SIZE>
void Grid<SIZE>::addNoise(int n)
{
    for (int i = 0; i < n; ++i) {
        const int x = rand() % SIZE;
        const int y = rand() % SIZE;
        toggle({ x, y });
    }
}

// Function to clear the grid
template <int SIZE>
void Grid<SIZE>::clear()
{
    grid_.fill(false);
}
