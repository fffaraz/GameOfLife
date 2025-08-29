#pragma once

// Function to apply the rules of Conway's Game of Life
inline bool gameOfLife(const bool cell, const int liveNeighbours)
{
    if (cell) {
        if (liveNeighbours < 2 || liveNeighbours > 3)
            return false; // Kill the cell if it has less than 2 or more than 3 live neighbours
        return true; // Keep the cell alive if it has 2 or 3 live neighbours
    }
    if (liveNeighbours == 3)
        return true; // Revive the cell if it has exactly 3 live neighbours
    return false; // Keep the cell dead if it has less than 3 or more than 3 live neighbours
}
