// LevelDesigner.h
#pragma once

#include <SDL.h>
#include "TextureManager.h"

class LevelDesigner
{
public:
    // Match your window size: 1280 x 720, tiles of 32x32
    static const int TILE_SIZE = 32;
    static const int GRID_COLS = 1280 / TILE_SIZE; // 40
    static const int GRID_ROWS = 720 / TILE_SIZE; // 22 (with a bit of bottom margin)

    LevelDesigner();
    ~LevelDesigner();

    // Call once after renderer is created
    bool Init(SDL_Renderer* renderer);

    // Forward SDL events from main loop to here
    void HandleEvent(const SDL_Event& e);

    // Draw tiles + grid
    void Render(SDL_Renderer* renderer);

private:
    struct Cell
    {
        bool filled = false;
        int  tileX = 0; // column index in tileset
        int  tileY = 0; // row index in tileset
    };

    Cell m_grid[GRID_ROWS][GRID_COLS]{};

    SDL_Texture* m_tileset = nullptr;

    // Which sprite from the tileset we are “painting” with
    int m_selectedTileX = 0;
    int m_selectedTileY = 0;
};
