// Conway's Game of Life using SDL3
// Copyright (c) 2025 Faraz Fallahi <fffaraz@gmail.com>

#include "Common.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

/* We will use this renderer to draw into this window every frame. */
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    printInfo();
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

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    }
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

static void SimStep()
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
SDL_AppResult SDL_AppIterate(void *appstate)
{
    SimStep();

    /* clear the window to the draw color. */
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    int aliveCount = 0;
    {
        const auto [currGrid, lock] = grid.readBuffer();
        for (int i = 0; i < GRID_SIZE; ++i) {
            for (int j = 0; j < GRID_SIZE; ++j) {
                if (currGrid.get({i, j})) {
                    aliveCount++;
                    SDL_RenderPoint(renderer, i, j);
                }
            }
        }
    }

    /* put the newly-cleared rendering on the screen. */
    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    /* SDL will clean up the window/renderer for us. */
}
