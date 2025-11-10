// Conway's Game of Life using raylib
// Copyright (c) 2025 Faraz Fallahi <fffaraz@gmail.com>

#include "Common.hpp"

#include <raylib.h>

#include <string>

static void SimStep()
{
    auto [nextGrid, writeLock] = grid.writeBuffer();
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        nextGrid.clear();
        grid.swap(std::move(writeLock));
        return;
    }
    {
        const auto [currGrid, readLock] = grid.readBuffer();
        nextGrid.updateGrid(currGrid);
    }
    nextGrid.addNoise();
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        const auto pos = GetMousePosition();
        nextGrid.toggleBlock({ static_cast<int>(pos.x), static_cast<int>(pos.y) });
    }
    grid.swap(std::move(writeLock));
}

int main()
{
    printInfo();

    // Initialization
    InitWindow(GRID_SIZE * CELL_SIZE, GRID_SIZE * CELL_SIZE, "Conway's Game of Life");

    if (targetFPS > 0) {
        SetTargetFPS(targetFPS);
    }

    // Main game loop
    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLACK);
        SimStep();
        int aliveCount = 0;
        {
            const auto [currGrid, lock] = grid.readBuffer();
            for (int i = 0; i < GRID_SIZE; ++i) {
                for (int j = 0; j < GRID_SIZE; ++j) {
                    if (currGrid.get({i, j})) {
                        aliveCount++;
                        DrawPixel(i, j, WHITE);
                    }
                }
            }
        }
        std::string aliveCountStr = "Alive: " + std::to_string(aliveCount);
        DrawText(aliveCountStr.c_str(), 10, 10, 20, LIGHTGRAY);
        EndDrawing();
    }

    // Close window and OpenGL context
    CloseWindow();        

    return 0;
}
