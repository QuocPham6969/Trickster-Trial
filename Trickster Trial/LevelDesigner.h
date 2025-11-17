#pragma once

#include <SDL.h>
#include "TextureManager.h"

class LevelDesigner
{
public:
    // size of 1 grid box on screen
    static const int TILE_SIZE = 32;

    // size of 1 tile inside the tileset image
    static const int SRC_TILE_SIZE = 64;

    static const int GRID_COLS = 1280 / TILE_SIZE;
    static const int GRID_ROWS = 720 / TILE_SIZE;

    LevelDesigner();
    ~LevelDesigner();

    bool Init(SDL_Renderer* renderer);
    void HandleEvent(const SDL_Event& e);
    void Render(SDL_Renderer* renderer);

private:
    struct Cell {
        bool filled = false;
        int  tileX = 0;
        int  tileY = 0;
    };

    Cell m_grid[GRID_ROWS][GRID_COLS]{};

    SDL_Texture* m_tileset = nullptr;
    int m_selectedTileX = 0;
    int m_selectedTileY = 0;
};
