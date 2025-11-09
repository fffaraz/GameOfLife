#include "DoubleBuffer.hpp"
#include "Grid.hpp"

#include <iostream>

constexpr int GRID_SIZE = 1024; // Size of the grid in cells
constexpr int CELL_SIZE = 1; // Size of each cell in pixels

DoubleBuffer<Grid<GRID_SIZE>> grid;

void printInfo()
{
    std::cout << "Conway's Game of Life\n";
    std::cout << "https://github.com/fffaraz/GameOfLife\n";
    std::cout << "Grid size: " << GRID_SIZE << " x " << GRID_SIZE << "\n";
    std::cout << "Cell size: " << CELL_SIZE << " pixels\n";
    std::cout << "Hardware concurrency: " << std::thread::hardware_concurrency() << "\n";
}
