// Conway's Game of Life
// Copyright (c) 2025 Faraz Fallahi <fffaraz@gmail.com>

#include <SFML/Graphics.hpp>

#include "DoubleBuffer.hpp"
#include "Grid.hpp"

#include <iostream>
#include <thread>

constexpr int GRID_SIZE = 512; // Size of the grid in cells
constexpr int CELL_SIZE = 2; // Size of each cell in pixels

DoubleBuffer<Grid<GRID_SIZE>> grid;

#if 1
// Update the next grid in place
static void updateGrid(sf::RenderWindow& window)
{
    // Get the next grid to write to
    auto [nextGrid, writeLock] = grid.writeBuffer();

    // Handle right mouse button click
    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right)) {
        nextGrid.clear(); // Clear the grid
        grid.swap(std::move(writeLock));
        return;
    }

    // Get the current grid and update nextGrid - scope ensures readLock is released
    {
        const auto [currGrid, readLock] = grid.readBuffer();
        nextGrid.update(currGrid);
    }

    // Add some noise to the grid
    nextGrid.addNoise();

    // Handle mouse movement while the left button is pressed
    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
        const sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        const int x = mousePos.x / CELL_SIZE;
        const int y = mousePos.y / CELL_SIZE;
        if (x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE) {
            nextGrid.toggleBlock({ x, y }); // Turn on a 3x3 block
        }
    }

    // Swap the buffers
    grid.swap(std::move(writeLock));
}
#else
// Update the next grid by creating a new one and swapping
static void updateGrid(sf::RenderWindow& window)
{
    Grid<GRID_SIZE> nextGrid;

    // Handle right mouse button click
    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right)) {
        nextGrid.clear(); // Clear the grid
        grid.setAndSwap(std::move(nextGrid));
        return;
    }

    // Get the current grid and update nextGrid - scope ensures readLock is released
    {
        const auto [currGrid, readLock] = grid.readBuffer();
        nextGrid.update(currGrid);
    }

    // Add some noise to the grid
    nextGrid.addNoise();

    // Handle mouse movement while the left button is pressed
    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
        const sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        const int x = mousePos.x / CELL_SIZE;
        const int y = mousePos.y / CELL_SIZE;
        if (x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE) {
            nextGrid.toggleBlock({ x, y }); // Turn on a 3x3 block
        }
    }

    // Set the new grid and swap the buffers
    grid.setAndSwap(std::move(nextGrid));
}
#endif

// Returns cell color based on the number of live neighbours
inline sf::Color getCellColor(int liveNeighbours)
{
    switch (liveNeighbours) {
    case 0:
        return sf::Color::Red;
    case 1:
        return sf::Color::Green;
    case 2:
        return sf::Color::Blue;
    case 3:
        return sf::Color::Cyan;
    case 4:
        return sf::Color::Magenta;
    case 5:
        return sf::Color::Yellow;
    default:
        return sf::Color::White;
    }
}

// Function to update the vertex array
int updateVertices(sf::RenderWindow& window, sf::VertexArray& vertices)
{
    int numAlive = 0; // Count the number of alive cells
    auto [currGrid, lock] = grid.readBuffer();
    for (int i = 0; i < GRID_SIZE; ++i) {
        for (int j = 0; j < GRID_SIZE; ++j) {
            const Point p { i, j };
            const bool cellAlive = currGrid.get(p);
            numAlive += cellAlive ? 1 : 0;
            const sf::Color color = cellAlive ? getCellColor(currGrid.countLiveNeighbours(p)) : sf::Color::Black;
            const int index = ((i * GRID_SIZE) + j) * 6;
            vertices[index + 0].color = color;
            vertices[index + 1].color = color;
            vertices[index + 2].color = color;
            vertices[index + 3].color = color;
            vertices[index + 4].color = color;
            vertices[index + 5].color = color;
        }
    }
    return numAlive; // Return the number of alive cells
}

int main()
{
    std::cout << "Conway's Game of Life\n";
    std::cout << "https://github.com/fffaraz/GameOfLife\n";
    std::cout << "SFML version: " << SFML_VERSION_MAJOR << "." << SFML_VERSION_MINOR << "." << SFML_VERSION_PATCH << "\n";
    std::cout << "Grid size: " << GRID_SIZE << " x " << GRID_SIZE << "\n";
    std::cout << "Cell size: " << CELL_SIZE << " pixels\n";
    std::cout << "Hardware concurrency: " << std::thread::hardware_concurrency() << "\n";

    // Create the main window
    sf::RenderWindow window(sf::VideoMode({ GRID_SIZE * CELL_SIZE, GRID_SIZE * CELL_SIZE }), "Conway's Game of Life");
    if (1)
    {
        const int targetFPS = 30;
        window.setFramerateLimit(targetFPS);
        std::cout << "Target FPS: " << targetFPS << "\n";
    }
    std::cout.flush();

    // Vertex array for the grid
    sf::VertexArray vertices(sf::PrimitiveType::Triangles, GRID_SIZE * GRID_SIZE * 6);
    for (int i = 0; i < GRID_SIZE; ++i) {
        for (int j = 0; j < GRID_SIZE; ++j) {
            const int index = (i * GRID_SIZE + j) * 6;
            const float x = (float)i * CELL_SIZE;
            const float y = (float)j * CELL_SIZE;
            vertices[index + 0].position = sf::Vector2f(x, y);
            vertices[index + 1].position = sf::Vector2f(x + CELL_SIZE, y);
            vertices[index + 2].position = sf::Vector2f(x + CELL_SIZE, y + CELL_SIZE);
            vertices[index + 3].position = sf::Vector2f(x, y);
            vertices[index + 4].position = sf::Vector2f(x, y + CELL_SIZE);
            vertices[index + 5].position = sf::Vector2f(x + CELL_SIZE, y + CELL_SIZE);
        }
    }

    // Text to display the number of alive cells
    sf::Font font;
    const std::vector<std::string> fontPaths {
#ifdef _WIN32
        "C:\\Windows\\Fonts\\Arial.ttf",
#else
        "/usr/share/fonts/gnu-free/FreeSans.ttf",
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

    // Start the grid update thread
    std::atomic<float> epochsPerSecond = 0.0f;
    std::jthread updateThread([&window, &epochsPerSecond](std::stop_token stop_token) {
        sf::Clock epochClock;
        int epochCount = 0;
        while (!stop_token.stop_requested()) {
            updateGrid(window);
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
            }
        }

        // Update the grid
        const int numAlive = updateVertices(window, vertices);
        txtNumAlive.setString("Alive: " + std::to_string(numAlive));

        // Update FPS counter
        frameCount++;
        if (fpsClock.getElapsedTime().asSeconds() >= 1.0f) {
            const float fps = frameCount / fpsClock.getElapsedTime().asSeconds();
            char buffer[50];
            snprintf(buffer, sizeof(buffer), "FPS: %.2f\nEPS: %.2f", fps, epochsPerSecond.load());
            txtFPS.setString(buffer);
            frameCount = 0;
            fpsClock.restart();
        }

        // Clear the window
        window.clear();

        // Draw the grid and texts
        window.draw(vertices);
        window.draw(txtNumAlive);
        window.draw(txtFPS);

        // Update the window
        window.display();
    }

    // Stop the grid update thread
    updateThread.request_stop();

    return 0;
}
