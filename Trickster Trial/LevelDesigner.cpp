// LevelDesigner.cpp
#include "LevelDesigner.h"

LevelDesigner::LevelDesigner()
{
}

LevelDesigner::~LevelDesigner()
{
    if (m_tileset)
    {
        SDL_DestroyTexture(m_tileset);
        m_tileset = nullptr;
    }
}

bool LevelDesigner::Init(SDL_Renderer* renderer)
{
    auto& texMgr = TextureManager::Instance();

    // Make sure this path matches where Terrain32x32.png actually is
    m_tileset = texMgr.LoadTexture("assets/textures/Terrain32x32.png", renderer);
    if (!m_tileset)
    {
        SDL_Log("LevelDesigner: failed to load tileset");
        return false;
    }

    // Start with tile (0,0) from the sheet.
    // Once you know where the crater tile is, change these:
    m_selectedTileX = 0; // column index of the tile you want
    m_selectedTileY = 0; // row index of the tile you want

    return true;
}

void LevelDesigner::HandleEvent(const SDL_Event& e)
{
    // Left mouse button = paint/toggle tile
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
    {
        int mouseX = e.button.x;
        int mouseY = e.button.y;

        int col = mouseX / TILE_SIZE;
        int row = mouseY / TILE_SIZE;

        if (row >= 0 && row < GRID_ROWS && col >= 0 && col < GRID_COLS)
        {
            Cell& cell = m_grid[row][col];

            // toggle for now
            if (!cell.filled)
            {
                cell.filled = true;
                cell.tileX = m_selectedTileX;
                cell.tileY = m_selectedTileY;
            }
            else
            {
                cell.filled = false;
            }
        }
    }

    // OPTIONAL: change brush tile with keys later, e.g.:
    //
    // if (e.type == SDL_KEYDOWN) {
    //     if (e.key.keysym.sym == SDLK_1) { m_selectedTileX = 0; m_selectedTileY = 0; }
    //     if (e.key.keysym.sym == SDLK_2) { m_selectedTileX = 1; m_selectedTileY = 0; }
    // }
}

void LevelDesigner::Render(SDL_Renderer* renderer)
{
    auto& texMgr = TextureManager::Instance();

    // 1) Draw all tiles
    for (int row = 0; row < GRID_ROWS; ++row)
    {
        for (int col = 0; col < GRID_COLS; ++col)
        {
            const Cell& cell = m_grid[row][col];
            if (!cell.filled || !m_tileset)
                continue;

            int dstX = col * TILE_SIZE;
            int dstY = row * TILE_SIZE;

            // Draw a full 64x64 tile 1:1 into the 64x64 grid cell
            texMgr.DrawTile(
                m_tileset,
                renderer,
                TILE_SIZE,        // 64 in the sheet
                cell.tileX,
                cell.tileY,
                dstX,
                dstY,
                1                 // no extra scaling
            );
        }
    }

    // 2) Draw grid lines on top
    SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);

    // vertical lines
    for (int x = 0; x <= GRID_COLS * TILE_SIZE; x += TILE_SIZE)
    {
        SDL_RenderDrawLine(renderer, x, 0, x, GRID_ROWS * TILE_SIZE);
    }

    // horizontal lines
    for (int y = 0; y <= GRID_ROWS * TILE_SIZE; y += TILE_SIZE)
    {
        SDL_RenderDrawLine(renderer, 0, y, GRID_COLS * TILE_SIZE, y);
    }
}
