#pragma once

#include <SDL.h>
#include <string>        
#include "TextureManager.h"

class LevelDesigner
{
public:
    static const int TILE_SIZE_SCREEN = 64;  // how big each tile is in the editor
    static const int TILE_SIZE_SHEET = 32;  // actual tile size in the spritesheet

    static const int GRID_COLS = 1280 / TILE_SIZE_SCREEN;
    static const int GRID_ROWS = 720 / TILE_SIZE_SCREEN;

    LevelDesigner();
    ~LevelDesigner();

    bool Init(SDL_Renderer* renderer);
    void HandleEvent(const SDL_Event& e);
    void Render(SDL_Renderer* renderer);

    bool SaveToFile(const std::string& path) const;
    bool LoadFromFile(const std::string& path);

    void SetActiveLevel(int index);     // 0 or 1
    int  GetActiveLevel() const { return m_activeLevelIndex; }

    bool SaveCurrentLevel() const;
    bool LoadCurrentLevel();

    bool paintingEnabled = true;
    bool IsSolidCell(int col, int row) const;


private:

    static const int MAX_LEVELS = 2;

    std::string m_levelPaths[MAX_LEVELS];
    int         m_activeLevelIndex = 0;

    struct Cell
    {
        bool filled = false;
        int  tileX = 0; // which tile column in the sheet (0-based)
        int  tileY = 0; // which tile row in the sheet (0-based)
    };

    Cell m_grid[GRID_ROWS][GRID_COLS]{};

    SDL_Texture* m_tileset = nullptr;

    // Current "brush" tile index in the spritesheet
    int m_selectedTileX = 0;
    int m_selectedTileY = 0;

    bool m_isPainting = false;
    bool m_isErasing = false;

    // Apply brush to whatever cell is under (mouseX, mouseY)
    void ApplyBrush(int mouseX, int mouseY, bool erase);

};


