#pragma once
#include <string>
#include <SDL.h>

class TextureManager {
public:
    static TextureManager& Instance() {
        static TextureManager instance;
        return instance;
    }

    SDL_Texture* LoadTexture(const std::string& filePath, SDL_Renderer* renderer);

    void DrawFrame(SDL_Texture* texture,
        SDL_Renderer* renderer,
        int srcX, int srcY, int srcW, int srcH,
        int dstX, int dstY, int scale = 1);

    void DrawTile(SDL_Texture* texture,
        SDL_Renderer* renderer,
        int tileSize,
        int tileX, int tileY,
        int dstX, int dstY,
        int scale = 1);

    // NEW: src tile size and dst tile size can be different
    void DrawTileScaled(SDL_Texture* texture,
        SDL_Renderer* renderer,
        int srcTileSize,   // in texture (64)
        int dstTileSize,   // on screen (32)
        int tileX, int tileY,
        int dstX, int dstY);

private:
    TextureManager() = default;
};
