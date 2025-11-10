// Conway's Game of Life using raylib
// Copyright (c) 2025 Faraz Fallahi <fffaraz@gmail.com>

#include "Common.hpp"

#include <raylib.h>

int main(void)
{
    // Initialization
    InitWindow(GRID_SIZE * CELL_SIZE, GRID_SIZE * CELL_SIZE, "Conway's Game of Life");

    if (targetFPS > 0) {
        SetTargetFPS(targetFPS);
    }

    // Main game loop
    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Conway's Game of Life", 10, 10, 20, LIGHTGRAY);
        EndDrawing();
    }

    // Close window and OpenGL context
    CloseWindow();        

    return 0;
}
