// Conway's Game of Life using SFML
// Copyright (c) 2025 Faraz Fallahi <fffaraz@gmail.com>

#include "Common.hpp"

#include <SFML/Graphics.hpp>

#include <atomic>
#include <cstdint>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

std::atomic_bool mouseRightPressed = false;
std::atomic_bool mouseLeftPressed = false;

// Update the next grid state
static void updateGrid(GridType& grid, const sf::RenderWindow& window)
{
    // Get the writable next grid
    auto [nextGrid, writeLock] = grid.writeBuffer();

    // Handle right mouse button click
    if (mouseRightPressed.load()) {
        nextGrid.clear(); // Clear the grid
        grid.swap(std::move(writeLock));
        return;
    }

    // Get the current grid and update nextGrid - scope ensures readLock is released
    {
        const auto [currGrid, readLock] = grid.readBuffer();
        nextGrid.updateGrid(currGrid);
    }

    // Add random noise to the grid
    nextGrid.addNoise();

    // Handle mouse movement while the left button is pressed
    if (mouseLeftPressed.load()) {
        const sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        const int x = mousePos.x / CELL_SIZE;
        const int y = mousePos.y / CELL_SIZE;
        if (x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE) {
            nextGrid.toggleBlock({ x, y }); // Toggle a 3x3 block
        }
    }

    // Swap the buffers
    grid.swap(std::move(writeLock));
}

// Color map for cells based on the number of live neighbors
static const std::array<sf::Color, 9> colorMap {
    sf::Color::Red, // 0 live neighbors
    sf::Color::Green, // 1 live neighbor
    sf::Color::Blue, // 2 live neighbors
    sf::Color::Cyan, // 3 live neighbors
    sf::Color::Magenta, // 4 live neighbors
    sf::Color::Yellow, // 5 live neighbors
    sf::Color::White, // 6 live neighbors
    sf::Color::White, // 7 live neighbors
    sf::Color::White, // 8 live neighbors
};

// Rewrite the RGBA pixel buffer from the grid; returns the live-cell count. One
// pixel per cell (the texture is scaled up by CELL_SIZE at draw time), replacing
// the old per-cell vertex array of up to 6 vertices per cell.
int fillPixels(GridType& grid, std::vector<std::uint8_t>& pixels)
{
    int numAlive = 0;
    const auto [currGrid, lock] = grid.readBuffer();
    for (int j = 0; j < GRID_SIZE; ++j) { // texture row = screen y
        for (int i = 0; i < GRID_SIZE; ++i) { // texture column = screen x
            const Point p { i, j };
            const bool cellAlive = currGrid.get(p);
            numAlive += cellAlive ? 1 : 0;
#if 1 // Enable to color cells based on live neighbors
            const sf::Color color = cellAlive ? colorMap[currGrid.countLiveNeighbors(p)] : sf::Color::Black;
#else
            const sf::Color color = cellAlive ? sf::Color::White : sf::Color::Black;
#endif
            const std::size_t idx = ((static_cast<std::size_t>(j) * GRID_SIZE) + i) * 4;
            pixels[idx + 0] = color.r;
            pixels[idx + 1] = color.g;
            pixels[idx + 2] = color.b;
            pixels[idx + 3] = color.a;
        }
    }
    return numAlive; // Return the number of alive cells
}

int main()
{
    printAppInfo();
    std::cout << "SFML version: " << SFML_VERSION_MAJOR << "." << SFML_VERSION_MINOR << "." << SFML_VERSION_PATCH << "\n";

    // Create the main window
    sf::RenderWindow window(sf::VideoMode({ GRID_SIZE * CELL_SIZE, GRID_SIZE * CELL_SIZE }), "Conway's Game of Life");
    if (targetFPS > 0) {
        window.setFramerateLimit(targetFPS);
        std::cout << "Framerate Limit: " << targetFPS << "\n";
    }
    std::cout.flush();

    // One texture holds the whole grid at 1 pixel/cell, scaled up by CELL_SIZE via
    // the sprite. Rewritten and uploaded once per frame instead of a per-cell mesh.
    sf::Texture texture;
    if (!texture.resize({ GRID_SIZE, GRID_SIZE })) {
        std::cerr << "Failed to create grid texture\n";
        return 1;
    }
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(GRID_SIZE) * GRID_SIZE * 4);
    sf::Sprite sprite(texture);
    sprite.setScale({ static_cast<float>(CELL_SIZE), static_cast<float>(CELL_SIZE) });

    // Load font for displaying text
    sf::Font font;
    const std::vector<std::string> fontPaths {
#ifdef _WIN32
        "C:\\Windows\\Fonts\\Arial.ttf",
#else
        "/usr/share/fonts/gnu-free/FreeSans.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
        "/usr/share/fonts/truetype/msttcorefonts/arial.ttf",
#endif
    };
    for (const auto& path : fontPaths) {
        if (font.openFromFile(path)) {
            break;
        }
    }

    sf::Text txtNumAlive(font, "", 24);
    txtNumAlive.setFillColor(sf::Color::White);
    txtNumAlive.setPosition({ 10, 5 });
    txtNumAlive.setOutlineThickness(2);
    txtNumAlive.setOutlineColor(sf::Color::Black);

    sf::Text txtFPS(font, "", 24);
    txtFPS.setFillColor(sf::Color::White);
    txtFPS.setPosition({ (float)window.getSize().x - 200, 5 });
    txtFPS.setOutlineThickness(2);
    txtFPS.setOutlineColor(sf::Color::Black);

    // Heap-allocated: a GridType holds two grids inline (~8 MB at GRID_SIZE 2048),
    // which overflows the stack if placed as a local.
    auto gridPtr = std::make_unique<GridType>();
    GridType& grid = *gridPtr;

    // Start the grid update thread
    std::atomic<float> epochsPerSecond = 0.0f;
    std::jthread updateThread([&grid, &window, &epochsPerSecond](std::stop_token stop_token) {
        sf::Clock epochClock;
        int epochCount = 0;
        while (!stop_token.stop_requested()) {
            updateGrid(grid, window);
            epochCount++;
            if (epochClock.getElapsedTime().asSeconds() >= 1.0f) {
                epochsPerSecond = epochCount / epochClock.getElapsedTime().asSeconds();
                epochCount = 0;
                epochClock.restart();
            }
        }
    });

    // Clock for FPS calculation
    sf::Clock fpsClock;
    int frameCount = 0;

    // Start the game loop
    while (window.isOpen()) {
        // Process events
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            } else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) {
                    window.close();
                }
            } else if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mousePressed->button == sf::Mouse::Button::Left) {
                    mouseLeftPressed.store(true);
                } else if (mousePressed->button == sf::Mouse::Button::Right) {
                    mouseRightPressed.store(true);
                }
            } else if (const auto* mouseReleased = event->getIf<sf::Event::MouseButtonReleased>()) {
                if (mouseReleased->button == sf::Mouse::Button::Left) {
                    mouseLeftPressed.store(false);
                } else if (mouseReleased->button == sf::Mouse::Button::Right) {
                    mouseRightPressed.store(false);
                }
            }
        }

        // Rebuild the grid texture from the latest state.
        const int numAlive = fillPixels(grid, pixels);
        texture.update(pixels.data());
        txtNumAlive.setString("Alive: " + std::to_string(numAlive));

        // Update FPS counter
        frameCount++;
        if (fpsClock.getElapsedTime().asSeconds() >= 1.0f) {
            const float fps = frameCount / fpsClock.getElapsedTime().asSeconds();
            const float eps = epochsPerSecond.load();
            const float cups = eps * (GRID_SIZE * GRID_SIZE / 1'000'000'000.0);
            char buffer[50];
            snprintf(buffer, sizeof(buffer), "FPS: %.2f\nEPS: %.2f\nCUpS: %.3fe9", fps, eps, cups);
            txtFPS.setString(buffer);
            frameCount = 0;
            fpsClock.restart();
        }

        // Clear the window
        window.clear();

        // Draw the grid and texts
        window.draw(sprite);
        window.draw(txtNumAlive);
        window.draw(txtFPS);

        // Update the window
        window.display();
    }

    // Stop the grid update thread
    updateThread.request_stop();

    return 0;
}
