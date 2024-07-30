#include <SFML/Graphics.hpp>

#include <atomic>
#include <iostream>
#include <mutex>
#include <thread>

// Grid size
const int GRID_SIZE = 100; // Size of the grid in cells
const int CELL_SIZE = 10; // Size of each cell in pixels

bool grid1[GRID_SIZE * GRID_SIZE];
bool grid2[GRID_SIZE * GRID_SIZE];
std::atomic<bool*> gridPtr = grid1;
std::mutex gridMutex;

#define GRID(x, y) grid[(x * GRID_SIZE) + y]
#define NEXT_GRID(x, y) nextGrid[(x * GRID_SIZE) + y]

// Function to count live neighbours of a cell
int countLiveNeighbours(const bool* grid, int x, int y)
{
    int liveNeighbours = 0;
    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            if (i == 0 && j == 0)
                continue; // Skip the cell itself
            const int nx = x + i;
            const int ny = y + j;
            if (nx >= 0 && nx < GRID_SIZE && ny >= 0 && ny < GRID_SIZE) {
                if (GRID(nx, ny)) {
                    ++liveNeighbours;
                }
            }
        }
    }
    return liveNeighbours;
}

// Function to apply the rules of Conway's Game of Life
inline bool gameOfLife(const bool cell, const int liveNeighbours)
{
    if (cell) {
        if (liveNeighbours < 2 || liveNeighbours > 3) {
            return false; // Kill the cell if it has less than 2 or more than 3 live neighbours
        } else {
            return true; // Keep the cell alive if it has 2 or 3 live neighbours
        }
    } else {
        if (liveNeighbours == 3) {
            return true; // Revive the cell if it has exactly 3 live neighbours
        } else {
            return false; // Keep the cell dead if it has less than 3 or more than 3 live neighbours
        }
    }
}

void updateGrid()
{
    // Get the current grid and the next grid
    std::lock_guard<std::mutex> lock(gridMutex);
    const bool* grid = gridPtr.load();
    bool* const nextGrid = grid == grid1 ? grid2 : grid1;

    // Update the grid
    for (int i = 0; i < GRID_SIZE; ++i) {
        for (int j = 0; j < GRID_SIZE; ++j) {
            // Randomly kill or revive a cell with a 1 in 8192 chance, otherwise apply the rules of the game
            switch (rand() & ((1 << 13) - 1)) {
            case 0:
                NEXT_GRID(i, j) = true;
                break;
            case 1:
                NEXT_GRID(i, j) = false;
                break;
            default:
                NEXT_GRID(i, j) = gameOfLife(GRID(i, j), countLiveNeighbours(grid, i, j));
                break;
            }
        }
    }

    // Swap the grids
    gridPtr.store(nextGrid);
}

// Thread function to update the grid
void threadFunc(std::stop_token stop_token)
{
    while (!stop_token.stop_requested()) {
        updateGrid();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// Function to toggle a 3x3 block of cells at the given position
void toggleCell(bool* grid, int x, int y)
{
    const int size = 1;
    for (int i = -size; i <= size; ++i) {
        for (int j = -size; j <= size; ++j) {
            int nx = x + i;
            int ny = y + j;
            if (nx >= 0 && nx < GRID_SIZE && ny >= 0 && ny < GRID_SIZE) {
                GRID(nx, ny) = !GRID(nx, ny); // Toggle cell state
            }
        }
    }
}

// Function to update the vertex array and handle mouse input
int updateVertices(sf::RenderWindow& window, sf::VertexArray& vertices)
{
    // Lock the grid mutex and get the current grid
    std::lock_guard<std::mutex> lock(gridMutex);
    bool* grid = gridPtr.load();

    // Handle mouse movement while the left button is pressed
    if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
        const sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        const int x = mousePos.x / CELL_SIZE;
        const int y = mousePos.y / CELL_SIZE;
        if (x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE) {
            toggleCell(grid, x, y); // Turn on a 3x3 block
        }
    }

    // Count the number of alive cells
    int alive = 0;

    // Update the vertex array
    for (int i = 0; i < GRID_SIZE; ++i) {
        for (int j = 0; j < GRID_SIZE; ++j) {
            sf::Color color = sf::Color::Black;
            if (GRID(i, j)) {
                ++alive;
                const int liveNeighbours = countLiveNeighbours(grid, i, j);
                switch (liveNeighbours) {
                case 0:
                    color = sf::Color::White;
                    break;
                case 1:
                    color = sf::Color::Red;
                    break;
                case 2:
                    color = sf::Color::Blue;
                    break;
                case 3:
                    color = sf::Color::Magenta;
                    break;
                default:
                    color = sf::Color::Green;
                    break;
                }
            }
            const int index = (i * GRID_SIZE + j) * 4;
            vertices[index + 0].color = color;
            vertices[index + 1].color = color;
            vertices[index + 2].color = color;
            vertices[index + 3].color = color;
        }
    }

    // Return the number of alive cells
    return alive;
}

int main()
{
    // Start the grid update thread
    std::jthread updateThread(threadFunc);

    // Create the main window
    sf::RenderWindow window(sf::VideoMode(GRID_SIZE * CELL_SIZE, GRID_SIZE * CELL_SIZE), "Conway's Game of Life");
    window.setFramerateLimit(100);

    // Vertex array for the grid
    sf::VertexArray vertices(sf::Quads, GRID_SIZE * GRID_SIZE * 4);
    for (int i = 0; i < GRID_SIZE; ++i) {
        for (int j = 0; j < GRID_SIZE; ++j) {
            const int index = (i * GRID_SIZE + j) * 4;
            const float x = i * CELL_SIZE;
            const float y = j * CELL_SIZE;

            vertices[index].position = sf::Vector2f(x, y);
            vertices[index + 1].position = sf::Vector2f(x + CELL_SIZE, y);
            vertices[index + 2].position = sf::Vector2f(x + CELL_SIZE, y + CELL_SIZE);
            vertices[index + 3].position = sf::Vector2f(x, y + CELL_SIZE);
        }
    }

    // Text to display the number of alive cells
    sf::Font font;
    if (!font.loadFromFile("/usr/share/fonts/truetype/msttcorefonts/arial.ttf")) {
        std::cerr << "Failed to load font file" << std::endl;
        return 1;
    }
    sf::Text text("0", font, 24);
    text.setFillColor(sf::Color::White);
    text.setPosition(10, 5);
    text.setOutlineThickness(2);
    text.setOutlineColor(sf::Color::Black);

    // Start the game loop
    while (window.isOpen()) {
        // Process events
        for (auto event = sf::Event {}; window.pollEvent(event);) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        // Update the grid
        const int alive = updateVertices(window, vertices);
        text.setString(std::to_string(alive));

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
