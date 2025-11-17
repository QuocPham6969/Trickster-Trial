#include "LevelDesigner.h"

LevelDesigner::LevelDesigner() {}
LevelDesigner::~LevelDesigner()
{
    if (m_tileset) {
        SDL_DestroyTexture(m_tileset);
        m_tileset = nullptr;
    }
}

bool LevelDesigner::Init(SDL_Renderer* renderer)
{
    auto& texMgr = TextureManager::Instance();

    m_tileset = texMgr.LoadTexture("assets/textures/Terrain32x32.png", renderer);
    if (!m_tileset) {
        SDL_Log("LevelDesigner: failed to load tileset");
        return false;
    }

    m_selectedTileX = 0;
    m_selectedTileY = 0;
    return true;
}

void LevelDesigner::HandleEvent(const SDL_Event& e)
{
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
    {
        int mouseX = e.button.x;
        int mouseY = e.button.y;

        int col = mouseX / TILE_SIZE;
        int row = mouseY / TILE_SIZE;

        if (row >= 0 && row < GRID_ROWS && col >= 0 && col < GRID_COLS)
        {
            Cell& cell = m_grid[row][col];
            cell.filled = !cell.filled;
            cell.tileX = m_selectedTileX;
            cell.tileY = m_selectedTileY;
        }
    }
}

void LevelDesigner::Render(SDL_Renderer* renderer)
{
    auto& texMgr = TextureManager::Instance();

    // Draw tiles
    for (int row = 0; row < GRID_ROWS; ++row)
    {
        for (int col = 0; col < GRID_COLS; ++col)
        {
            const Cell& cell = m_grid[row][col];
            if (!cell.filled || !m_tileset) continue;

            int dstX = col * TILE_SIZE;
            int dstY = row * TILE_SIZE;

            texMgr.DrawTileScaled(
                m_tileset,
                renderer,
                SRC_TILE_SIZE,   // 64 in texture
                TILE_SIZE,       // 32 on screen
                cell.tileX,
                cell.tileY,
                dstX,
                dstY
            );
        }
    }

    // Draw grid
    SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
    for (int x = 0; x <= GRID_COLS * TILE_SIZE; x += TILE_SIZE)
        SDL_RenderDrawLine(renderer, x, 0, x, GRID_ROWS * TILE_SIZE);
    for (int y = 0; y <= GRID_ROWS * TILE_SIZE; y += TILE_SIZE)
        SDL_RenderDrawLine(renderer, 0, y, GRID_COLS * TILE_SIZE, y);
}
