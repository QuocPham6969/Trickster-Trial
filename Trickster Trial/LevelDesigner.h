// LevelDesigner.h
#pragma once

#include <SDL.h>
#include "TextureManager.h"

class LevelDesigner
{
public:
    // Size of 1 grid cell AND 1 tile on screen
    static const int TILE_SIZE = 64;

    static const int GRID_COLS = 1280 / TILE_SIZE;
    static const int GRID_ROWS = 720 / TILE_SIZE;

    LevelDesigner();
    ~LevelDesigner();

    bool Init(SDL_Renderer* renderer);
    void HandleEvent(const SDL_Event& e);
    void Render(SDL_Renderer* renderer);

private:
    struct Cell
    {
        bool filled = false;
        int  tileX = 0; // which 64x64 tile in the sheet (column)
        int  tileY = 0; // which 64x64 tile in the sheet (row)
    };

    Cell m_grid[GRID_ROWS][GRID_COLS]{};

    SDL_Texture* m_tileset = nullptr;

    // Current "brush" tile index in the spritesheet
    int m_selectedTileX = 0;
    int m_selectedTileY = 0;
};
