#pragma once

#include <array>

#if 1
#include <execution>
#include <numeric>
#define PARALLEL_GRID 1
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
    void toggleBlock(const Point& p);
    void update(const Grid<SIZE>& current);
    void addNoise();
    void clear();

private:
    std::array<bool, SIZE * SIZE> grid_;
    inline static int index(const Point& p) { return (p.x * SIZE) + p.y; }
#ifdef PARALLEL_GRID
    static constexpr std::array<int, SIZE> indices = [](){ std::array<int, SIZE> v; std::iota(v.begin(), v.end(), 0); return v; }();
#endif
};

// Function to count the number of live neighbours for a cell at (x, y)
template <int SIZE>
int Grid<SIZE>::countLiveNeighbours(const Point& p) const
{
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

#ifdef PARALLEL_GRID

// Parallel version of the update function
template <int SIZE>
void Grid<SIZE>::update(const Grid<SIZE>& current)
{
    std::for_each(std::execution::par, indices.begin(), indices.end(),
        [&](int x) {
            for (int y = 0; y < SIZE; ++y) {
                const Point p { x, y };
                set(p, gameOfLife(current.get(p), current.countLiveNeighbours(p)));
            }
        });
}

#else

// Function to update the grid based on the rules of Conway's Game of Life
template <int SIZE>
void Grid<SIZE>::update(const Grid<SIZE>& current)
{
    for (int x = 0; x < SIZE; ++x) {
        for (int y = 0; y < SIZE; ++y) {
            const Point p { x, y };
            set(p, gameOfLife(current.get(p), current.countLiveNeighbours(p)));
        }
    }
}

#endif

// Function to add random noise to the grid
template <int SIZE>
void Grid<SIZE>::addNoise()
{
    for (int i = 0; i < 1; ++i) {
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
