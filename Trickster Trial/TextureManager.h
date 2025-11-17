#pragma once
#include <string>
#include <SDL.h>

class TextureManager {
public:
    static TextureManager& Instance() {
        static TextureManager instance;
        return instance;
    }

    // Load image and return SDL_Texture*
    SDL_Texture* LoadTexture(const std::string& filePath, SDL_Renderer* renderer);

    // Draw a generic sub-rectangle of the texture
    void DrawFrame(SDL_Texture* texture,
        SDL_Renderer* renderer,
        int srcX, int srcY, int srcW, int srcH,
        int dstX, int dstY, int scale = 1);

    // Draw a tile from a uniform tile sheet (like Terrain 32x32)
    void DrawTile(SDL_Texture* texture,
        SDL_Renderer* renderer,
        int tileSize,      // 32 for this sheet
        int tileX,         // column index
        int tileY,         // row index
        int dstX, int dstY,
        int scale = 1);

private:
    TextureManager() = default;
};
