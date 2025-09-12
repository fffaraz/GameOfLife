#include <SFML/Graphics.hpp>

#include "DoubleBuffer.hpp"
#include "Grid.hpp"

#include <array>
#include <atomic>
#include <iostream>
#include <mutex>
#include <thread>

constexpr int GRID_SIZE = 180; // Size of the grid in cells
constexpr int CELL_SIZE = 5; // Size of each cell in pixels

DoubleBuffer<Grid<GRID_SIZE>> grid;

static void updateGrid()
{
    // Get the current grid and the next grid
    auto [currGrid, nextGrid] = grid.get();
    grid.unlock();

    // Update the grid
    nextGrid.update(currGrid);
    nextGrid.addNoise();

    // Swap the grids
    grid.swap();
}

inline sf::Color getCellColor(int liveNeighbours)
{
    switch (liveNeighbours) {
    case 0:
        return sf::Color::White;
    case 1:
        return sf::Color::Red;
    case 2:
        return sf::Color::Blue;
    case 3:
        return sf::Color::Magenta;
    default:
        return sf::Color::Green;
    }
}

// Function to update the vertex array and handle mouse input
int updateVertices(sf::RenderWindow& window, sf::VertexArray& vertices)
{
    auto [currGrid, _] = grid.get();

    // Handle mouse movement while the left button is pressed
    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
        const sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        const int x = mousePos.x / CELL_SIZE;
        const int y = mousePos.y / CELL_SIZE;
        if (x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE) {
            currGrid.toggleBlock({ x, y }); // Turn on a 3x3 block
        }
    }

    // Handle right mouse button click
    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right)) {
        currGrid.clear(); // Clear the grid
    }

    // Count the number of alive cells
    int numAlive = 0;

    // Update the vertex array
    for (int i = 0; i < GRID_SIZE; ++i) {
        for (int j = 0; j < GRID_SIZE; ++j) {
            const bool cellAlive = currGrid.get({ i, j });
            if (cellAlive)
                ++numAlive;
            const sf::Color color = cellAlive ? getCellColor(currGrid.countLiveNeighbours({ i, j })) : sf::Color::Black;
            const int index = (i * GRID_SIZE + j) * 6;
            vertices[index + 0].color = color;
            vertices[index + 1].color = color;
            vertices[index + 2].color = color;
            vertices[index + 3].color = color;
            vertices[index + 4].color = color;
            vertices[index + 5].color = color;
        }
    }

    // Unlock the double buffer
    grid.unlock();

    // Return the number of alive cells
    return numAlive;
}

int main()
{
    std::cout << "Conway's Game of Life\n";
    std::cout << "SFML version: " << SFML_VERSION_MAJOR << "." << SFML_VERSION_MINOR << "." << SFML_VERSION_PATCH << "\n";

    // Create the main window
    sf::RenderWindow window(sf::VideoMode({ GRID_SIZE * CELL_SIZE, GRID_SIZE * CELL_SIZE }), "Conway's Game of Life");
    window.setFramerateLimit(100);

    // Vertex array for the grid
    sf::VertexArray vertices(sf::PrimitiveType::Triangles, GRID_SIZE * GRID_SIZE * 6);
    for (int i = 0; i < GRID_SIZE; ++i) {
        for (int j = 0; j < GRID_SIZE; ++j) {
            const int index = (i * GRID_SIZE + j) * 6;
            const float x = i * CELL_SIZE;
            const float y = j * CELL_SIZE;

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
        "/usr/share/fonts/gnu-free/FreeSans.ttf",
        "/usr/share/fonts/truetype/msttcorefonts/arial.ttf",
        "C:\\Windows\\Fonts\\Arial.ttf"
    };
    for (const auto& path : fontPaths) {
        if (font.openFromFile(path)) {
            break;
        }
    }

    sf::Text text(font, "0", 24);
    text.setFillColor(sf::Color::White);
    text.setPosition({ 10, 5 });
    text.setOutlineThickness(2);
    text.setOutlineColor(sf::Color::Black);

    // Start the grid update thread
    std::jthread updateThread([](std::stop_token stop_token) {
        while (!stop_token.stop_requested()) {
            updateGrid();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

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
        text.setString(std::to_string(numAlive));

        // Clear the window
        window.clear();

        // Draw the grid
        window.draw(vertices);
        window.draw(text);

        // Update the window
        window.display();
    }

    // Stop the grid update thread
    updateThread.request_stop();

    return 0;
}
