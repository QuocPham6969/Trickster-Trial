#pragma once

#include <string>
#include <SDL.h>

class TextureManager
{
public:
    static TextureManager& Instance()
    {
        static TextureManager instance;
        return instance;
    }

    // Load an image 
    SDL_Texture* LoadTexture(const std::string& filePath, SDL_Renderer* renderer);

    // Draw an arbitrary frame (generic helper)
    void DrawFrame(SDL_Texture* texture,
        SDL_Renderer* renderer,
        int srcX, int srcY, int srcW, int srcH,
        int dstX, int dstY, int scale = 1);

    // Draw a tile from a uniform tile sheet:
    // - tileSize = size of tile in the sheet (64 here)
    // - tileX, tileY = indices in the sheet grid (0-based)
    void DrawTile(SDL_Texture* texture,
        SDL_Renderer* renderer,
        int tileSize,
        int tileX, int tileY,
        int dstX, int dstY,
        int scale = 1);

    void DrawTileScaled(SDL_Texture* texture,
        SDL_Renderer* renderer,
        int srcTileSize, int dstTileSize,
        int tileX, int tileY,
        int dstX, int dstY);

private:
    TextureManager() = default;
};
