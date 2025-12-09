#include "LevelDesigner.h"
#include <fstream>


LevelDesigner::LevelDesigner()
{
    // Level 0 = level1.map, Level 1 = level2.map
    m_levelPaths[0] = "assets/levels/level1.map";
    m_levelPaths[1] = "assets/levels/level2.map";
    m_activeLevelIndex = 0; // start on level 1
}

LevelDesigner::~LevelDesigner()
{
    SaveCurrentLevel();   // save whichever level is active

    if (m_tileset)
    {
        SDL_DestroyTexture(m_tileset);
        m_tileset = nullptr;
    }
}

bool LevelDesigner::Init(SDL_Renderer* renderer)
{
    auto& texMgr = TextureManager::Instance();

    m_tileset = texMgr.LoadTexture("assets/textures/Terrain.png", renderer);
    if (!m_tileset)
    {
        SDL_Log("LevelDesigner: failed to load tileset");
        return false;
    }

    m_selectedTileX = 0;
    m_selectedTileY = 0;

    // Try to load the currently active level.
    LoadCurrentLevel();   // if file doesn't exist yet, just logs and starts empty

    return true;
}

void LevelDesigner::HandleEvent(const SDL_Event& e)
{
    // This lets the rest of your game still use mouse/keyboard.
    if (!paintingEnabled)
        return;

    if (e.type == SDL_MOUSEBUTTONDOWN)
    {
        if (e.button.button == SDL_BUTTON_LEFT)
        {
            m_isPainting = true;
            ApplyBrush(e.button.x, e.button.y, false); // paint once on click
        }
        else if (e.button.button == SDL_BUTTON_RIGHT)
        {
            m_isErasing = true;
            ApplyBrush(e.button.x, e.button.y, true);  // erase once on click
        }
    }

    if (e.type == SDL_MOUSEBUTTONUP)
    {
        if (e.button.button == SDL_BUTTON_LEFT)
            m_isPainting = false;
        else if (e.button.button == SDL_BUTTON_RIGHT)
            m_isErasing = false;
    }

    if (e.type == SDL_MOUSEMOTION)
    {
        if (m_isPainting)
        {
            ApplyBrush(e.motion.x, e.motion.y, false);
        }
        else if (m_isErasing)
        {
            ApplyBrush(e.motion.x, e.motion.y, true);
        }
    }

    if (e.type == SDL_KEYDOWN)
    {
        switch (e.key.keysym.sym)
        {
        case SDLK_0:  m_selectedTileX = 0; m_selectedTileY = 0; break;
        case SDLK_1:  m_selectedTileX = 0; m_selectedTileY = 2; break;
        case SDLK_2:  m_selectedTileX = 2; m_selectedTileY = 0; break;
        case SDLK_3:  m_selectedTileX = 2; m_selectedTileY = 2; break;
        case SDLK_4:  m_selectedTileX = 1; m_selectedTileY = 0; break;
        case SDLK_5:  m_selectedTileX = 0; m_selectedTileY = 1; break;
        case SDLK_6:  m_selectedTileX = 2; m_selectedTileY = 1; break;
        case SDLK_7:  m_selectedTileX = 16; m_selectedTileY = 1; break;
        case SDLK_8:  m_selectedTileX = 15; m_selectedTileY = 0; break;
        case SDLK_9:  m_selectedTileX = 15; m_selectedTileY = 1; break;

        case SDLK_q:  m_selectedTileX = 0; m_selectedTileY = 6; break; 
        case SDLK_w:  m_selectedTileX = 0; m_selectedTileY = 8; break; 
        case SDLK_e:  m_selectedTileX = 2; m_selectedTileY = 6; break;  
        case SDLK_r:  m_selectedTileX = 2; m_selectedTileY = 8; break;  
        case SDLK_t:  m_selectedTileX = 1; m_selectedTileY = 6; break;  
        case SDLK_y:  m_selectedTileX = 0; m_selectedTileY = 7; break;  
        case SDLK_u:  m_selectedTileX = 2; m_selectedTileY = 7; break;  
        case SDLK_i:  m_selectedTileX = 1; m_selectedTileY = 8; break; 
        case SDLK_o:  m_selectedTileX = 1; m_selectedTileY = 7; break; 

        case SDLK_F1:
            SetActiveLevel(0);  // level 1
            break;

        case SDLK_F2:
            SetActiveLevel(1);  // level 2
            break;

        }

        SDL_Log("Brush tile changed to (%d,%d)", m_selectedTileX, m_selectedTileY);
    }
}


void LevelDesigner::Render(SDL_Renderer* renderer)
{
    auto& texMgr = TextureManager::Instance();

    // Draw all tiles
    for (int row = 0; row < GRID_ROWS; ++row)
    {
        for (int col = 0; col < GRID_COLS; ++col)
        {
            const Cell& cell = m_grid[row][col];
            if (!cell.filled || !m_tileset)
                continue;

            int dstX = col * TILE_SIZE_SCREEN;
            int dstY = row * TILE_SIZE_SCREEN;

           
            texMgr.DrawTileScaled(
                m_tileset,
                renderer,
                TILE_SIZE_SHEET,      
                TILE_SIZE_SCREEN,     
                cell.tileX,
                cell.tileY,
                dstX,
                dstY
            );
        }
    }

    // Draw grid lines on top
    SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);

    // vertical lines
    for (int x = 0; x <= GRID_COLS * TILE_SIZE_SCREEN; x += TILE_SIZE_SCREEN)
    {
        SDL_RenderDrawLine(renderer, x, 0, x, GRID_ROWS * TILE_SIZE_SCREEN);
    }

    // horizontal lines
    for (int y = 0; y <= GRID_ROWS * TILE_SIZE_SCREEN; y += TILE_SIZE_SCREEN)
    {
        SDL_RenderDrawLine(renderer, 0, y, GRID_COLS * TILE_SIZE_SCREEN, y);
    }
}

bool LevelDesigner::SaveToFile(const std::string& path) const
{
    std::ofstream out(path);
    if (!out)
    {
        SDL_Log("LevelDesigner: failed to open %s for writing", path.c_str());
        return false;
    }

    // First write dimensions in case you change them later
    out << GRID_ROWS << ' ' << GRID_COLS << '\n';

    // Then write each cell: filled, tileX, tileY
    for (int row = 0; row < GRID_ROWS; ++row)
    {
        for (int col = 0; col < GRID_COLS; ++col)
        {
            const Cell& cell = m_grid[row][col];
            int filledInt = cell.filled ? 1 : 0;

            out << filledInt << ' ' << cell.tileX << ' ' << cell.tileY << ' ';
        }
        out << '\n';
    }

    return true;
}

bool LevelDesigner::LoadFromFile(const std::string& path)
{
    std::ifstream in(path);
    if (!in)
    {
        // No file yet = first run. Not an error.
        SDL_Log("LevelDesigner: no existing level file %s, starting empty", path.c_str());
        return false;
    }

    int fileRows = 0, fileCols = 0;
    in >> fileRows >> fileCols;

    if (!in)
    {
        SDL_Log("LevelDesigner: failed to read header from %s", path.c_str());
        return false;
    }

    if (fileRows != GRID_ROWS || fileCols != GRID_COLS)
    {
        SDL_Log("LevelDesigner: size mismatch in %s (file %dx%d, grid %dx%d).",
            path.c_str(), fileRows, fileCols, GRID_ROWS, GRID_COLS);
        // We can still attempt to read min(file, grid) safely.
    }

    for (int row = 0; row < GRID_ROWS; ++row)
    {
        for (int col = 0; col < GRID_COLS; ++col)
        {
            int filledInt = 0;
            int tileX = 0;
            int tileY = 0;

            if (!(in >> filledInt >> tileX >> tileY))
            {
                // If file ends early, fill the rest with empty cells.
                m_grid[row][col].filled = false;
                m_grid[row][col].tileX = 0;
                m_grid[row][col].tileY = 0;
            }
            else
            {
                m_grid[row][col].filled = (filledInt != 0);
                m_grid[row][col].tileX = tileX;
                m_grid[row][col].tileY = tileY;
            }
        }
    }

    return true;
}

bool LevelDesigner::SaveCurrentLevel() const
{
    if (m_activeLevelIndex < 0 || m_activeLevelIndex >= MAX_LEVELS)
        return false;

    return SaveToFile(m_levelPaths[m_activeLevelIndex]);
}

bool LevelDesigner::LoadCurrentLevel()
{
    if (m_activeLevelIndex < 0 || m_activeLevelIndex >= MAX_LEVELS)
        return false;

    return LoadFromFile(m_levelPaths[m_activeLevelIndex]);
}

void LevelDesigner::SetActiveLevel(int index)
{
    if (index < 0 || index >= MAX_LEVELS)
        return;

    // Save the current level before switching
    SaveCurrentLevel();

    m_activeLevelIndex = index;

    // Load the new level (or start empty if file missing)
    if (!LoadCurrentLevel())
    {
        SDL_Log("LevelDesigner: starting new empty level at %s",
            m_levelPaths[m_activeLevelIndex].c_str());
    }

    SDL_Log("LevelDesigner: switched to level %d (%s)",
        m_activeLevelIndex + 1,
        m_levelPaths[m_activeLevelIndex].c_str());
}


bool LevelDesigner::IsSolidCell(int col, int row) const
{
    // Outside map = solid
    if (row < 0 || row >= GRID_ROWS || col < 0 || col >= GRID_COLS)
        return true;

    const Cell& cell = m_grid[row][col];

    // If nothing painted here = solid 
    if (!cell.filled)
        return true;

    if (cell.tileY >= 6 && cell.tileY <= 8 &&    // tiles filled on ground
		cell.tileX >= 0 && cell.tileX <= 8)      // tiles filled on ground
    {
        return false; // walkable
    }

    // everything else is solid
    return true;
}

void LevelDesigner::ApplyBrush(int mouseX, int mouseY, bool erase)
{
    int col = mouseX / TILE_SIZE_SCREEN;
    int row = mouseY / TILE_SIZE_SCREEN;

    if (row < 0 || row >= GRID_ROWS || col < 0 || col >= GRID_COLS)
        return;

    Cell& cell = m_grid[row][col];

    if (erase)
    {
        cell.filled = false;
    }
    else
    {
        cell.filled = true;
        cell.tileX = m_selectedTileX;
        cell.tileY = m_selectedTileY;
    }
}





