// Conway's Game of Life using raylib
// Copyright (c) 2025 Faraz Fallahi <fffaraz@gmail.com>

#include "Common.hpp"

#include <raylib.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <memory>
#include <string>
#include <thread>

static constexpr std::array<Color, 9> colorMap {
    Color { 230, 41, 55, 255 }, // RED      - 0 live neighbors
    Color { 0, 228, 48, 255 }, // GREEN    - 1 live neighbor
    Color { 0, 121, 241, 255 }, // BLUE     - 2 live neighbors
    Color { 102, 191, 255, 255 }, // SKYBLUE  - 3 live neighbors (cyan-ish)
    Color { 200, 122, 255, 255 }, // MAGENTA  - 4 live neighbors
    Color { 253, 249, 0, 255 }, // YELLOW   - 5 live neighbors
    Color { 255, 255, 255, 255 }, // WHITE    - 6 live neighbors
    Color { 255, 255, 255, 255 }, // WHITE    - 7 live neighbors
    Color { 255, 255, 255, 255 }, // WHITE    - 8 live neighbors
};

std::atomic_bool mouseRightPressed = false;
std::atomic_bool mouseLeftPressed = false;
std::atomic_int mouseX = 0;
std::atomic_int mouseY = 0;

static void updateGrid(GridType& grid)
{
    auto [nextGrid, writeLock] = grid.writeBuffer();

    if (mouseRightPressed.load()) {
        nextGrid.clear();
        grid.swap(std::move(writeLock));
        return;
    }

    {
        const auto [currGrid, readLock] = grid.readBuffer();
        nextGrid.updateGrid(currGrid);
    }

    nextGrid.addNoise();

    if (mouseLeftPressed.load()) {
        const int x = mouseX.load() / CELL_SIZE;
        const int y = mouseY.load() / CELL_SIZE;
        if (x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE) {
            nextGrid.toggleBlock({ x, y });
        }
    }

    grid.swap(std::move(writeLock));
}

static void DrawTextOutlined(const char* text, int x, int y, int fontSize, Color color, Color outline)
{
    DrawText(text, x - 1, y, fontSize, outline);
    DrawText(text, x + 1, y, fontSize, outline);
    DrawText(text, x, y - 1, fontSize, outline);
    DrawText(text, x, y + 1, fontSize, outline);
    DrawText(text, x, y, fontSize, color);
}

int main()
{
    printAppInfo();

    InitWindow(GRID_SIZE * CELL_SIZE, GRID_SIZE * CELL_SIZE, "Conway's Game of Life");

    if (targetFPS > 0) {
        SetTargetFPS(targetFPS);
    }

    // Heap-allocated: a GridType holds two grids inline (~8 MB at GRID_SIZE 2048),
    // which overflows the stack if placed as a local.
    auto gridPtr = std::make_unique<GridType>();
    GridType& grid = *gridPtr;

    std::atomic<float> epochsPerSecond = 0.0f;
    std::jthread updateThread([&grid, &epochsPerSecond](std::stop_token stop_token) {
        auto epochStart = std::chrono::steady_clock::now();
        int epochCount = 0;
        while (!stop_token.stop_requested()) {
            updateGrid(grid);
            epochCount++;
            const auto now = std::chrono::steady_clock::now();
            const float elapsed = std::chrono::duration<float>(now - epochStart).count();
            if (elapsed >= 1.0f) {
                epochsPerSecond = epochCount / elapsed;
                epochCount = 0;
                epochStart = now;
            }
        }
    });

    while (!WindowShouldClose()) {
        mouseLeftPressed.store(IsMouseButtonDown(MOUSE_BUTTON_LEFT));
        mouseRightPressed.store(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT));
        const Vector2 pos = GetMousePosition();
        mouseX.store(static_cast<int>(pos.x));
        mouseY.store(static_cast<int>(pos.y));

        BeginDrawing();
        ClearBackground(BLACK);

        int aliveCount = 0;
        {
            const auto [currGrid, lock] = grid.readBuffer();
            for (int i = 0; i < GRID_SIZE; ++i) {
                for (int j = 0; j < GRID_SIZE; ++j) {
                    const Point p { i, j };
                    if (currGrid.get(p)) {
                        aliveCount++;
                        DrawPixel(i, j, colorMap[currGrid.countLiveNeighbors(p)]);
                    }
                }
            }
        }

        const std::string aliveStr = "Alive: " + std::to_string(aliveCount);
        DrawTextOutlined(aliveStr.c_str(), 10, 5, 24, WHITE, BLACK);

        const float fps = static_cast<float>(GetFPS());
        const float eps = epochsPerSecond.load();
        const float cups = eps * (GRID_SIZE * GRID_SIZE / 1'000'000'000.0f);
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "FPS: %.2f\nEPS: %.2f\nCUpS: %.3fe9", fps, eps, cups);
        DrawTextOutlined(buffer, GetScreenWidth() - 200, 5, 24, WHITE, BLACK);

        EndDrawing();
    }

    updateThread.request_stop();

    CloseWindow();

    return 0;
}
