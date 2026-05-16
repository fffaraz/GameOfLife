// Conway's Game of Life
// Copyright (c) 2025 Faraz Fallahi <fffaraz@gmail.com>

#pragma once

#include "DoubleBuffer.hpp"
#include "Grid.hpp"

constexpr int GRID_SIZE = 512; // Size of the grid in cells
constexpr int CELL_SIZE = 1; // Size of each cell in pixels
constexpr int targetFPS = 30;

void printAppInfo();
