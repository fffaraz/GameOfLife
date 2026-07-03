// Conway's Game of Life using SDL3
// Copyright (c) 2025 Faraz Fallahi <fffaraz@gmail.com>

#include "Common.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

/* We will use this renderer to draw into this window every frame. */
static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Texture* texture = NULL;

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    printAppInfo();
    SDL_SetAppMetadata("Conway's Game of Life", "1.0", "com.example.gameoflife");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("Conway's Game of Life", GRID_SIZE * CELL_SIZE, GRID_SIZE * CELL_SIZE, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(renderer, GRID_SIZE * CELL_SIZE, GRID_SIZE * CELL_SIZE, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    /* Streaming texture holds the grid at 1 pixel/cell, scaled to fill the window
       each frame. Replaces per-cell SDL_RenderPoint calls with one upload + blit. */
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, GRID_SIZE, GRID_SIZE);
    if (!texture) {
        SDL_Log("Couldn't create grid texture: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST); /* crisp square cells */

    return SDL_APP_CONTINUE; /* carry on with the program! */
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS; /* end the program, reporting success to the OS. */
    }
    return SDL_APP_CONTINUE; /* carry on with the program! */
}

static void SimStep(GridType& grid)
{
    float xpos = 0;
    float ypos = 0;
    const SDL_MouseButtonFlags btn = SDL_GetMouseState(&xpos, &ypos);

    auto [nextGrid, writeLock] = grid.writeBuffer();
    if (btn & SDL_BUTTON_RMASK) {
        nextGrid.clear();
        grid.swap(std::move(writeLock));
        return;
    }
    {
        const auto [currGrid, readLock] = grid.readBuffer();
        nextGrid.updateGrid(currGrid);
    }
    nextGrid.addNoise();
    if (btn & SDL_BUTTON_LMASK) {
        nextGrid.toggleBlock({ static_cast<int>(xpos), static_cast<int>(ypos) });
    }
    grid.swap(std::move(writeLock));
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void* appstate)
{
    static GridType grid;
    SimStep(grid);

    /* Rewrite the grid texture: white = alive, black = dead (ARGB8888). */
    void* texPixels = NULL;
    int pitch = 0;
    if (SDL_LockTexture(texture, NULL, &texPixels, &pitch)) {
        const auto [currGrid, lock] = grid.readBuffer();
        for (int j = 0; j < GRID_SIZE; ++j) { /* texture row = screen y */
            Uint32* const row = reinterpret_cast<Uint32*>(static_cast<Uint8*>(texPixels) + (j * pitch));
            for (int i = 0; i < GRID_SIZE; ++i) { /* column = screen x */
                row[i] = currGrid.get({ i, j }) ? 0xFFFFFFFFu : 0xFF000000u;
            }
        }
        SDL_UnlockTexture(texture);
    }

    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, texture, NULL, NULL); /* scale to fill the window */
    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE; /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    SDL_DestroyTexture(texture);
    /* SDL will clean up the window/renderer for us. */
}
