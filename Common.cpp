// Conway's Game of Life
// Copyright (c) 2025 Faraz Fallahi <fffaraz@gmail.com>

#include "Common.hpp"

#include <iostream>
#include <thread>

void printAppInfo()
{
    std::cout << "Conway's Game of Life\n";
    std::cout << "https://github.com/fffaraz/GameOfLife\n";
    std::cout << "Grid size: " << GRID_SIZE << " x " << GRID_SIZE << "\n";
    std::cout << "Cell size: " << CELL_SIZE << " pixels\n";
    std::cout << "Hardware concurrency: " << std::thread::hardware_concurrency() << "\n";
}
