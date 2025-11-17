#include "TextureManager.h"
#include <SDL_image.h>

SDL_Texture* TextureManager::LoadTexture(const std::string& filePath, SDL_Renderer* renderer)
{
    SDL_Texture* tex = IMG_LoadTexture(renderer, filePath.c_str());
    if (!tex) {
        SDL_Log("Failed to load texture %s: %s", filePath.c_str(), IMG_GetError());
    }
    return tex;
}

void TextureManager::DrawFrame(SDL_Texture* texture,
    SDL_Renderer* renderer,
    int srcX, int srcY, int srcW, int srcH,
    int dstX, int dstY, int scale)
{
    if (!texture) return;

    SDL_Rect srcRect{ srcX, srcY, srcW, srcH };
    SDL_Rect dstRect{ dstX, dstY, srcW * scale, srcH * scale };

    SDL_RenderCopy(renderer, texture, &srcRect, &dstRect);
}

void TextureManager::DrawTile(SDL_Texture* texture,
    SDL_Renderer* renderer,
    int tileSize,
    int tileX, int tileY,
    int dstX, int dstY,
    int scale)
{
    if (!texture) return;

    SDL_Rect srcRect;
    srcRect.x = tileX * tileSize;
    srcRect.y = tileY * tileSize;
    srcRect.w = tileSize;
    srcRect.h = tileSize;

    SDL_Rect dstRect;
    dstRect.x = dstX;
    dstRect.y = dstY;
    dstRect.w = tileSize * scale;
    dstRect.h = tileSize * scale;

    SDL_RenderCopy(renderer, texture, &srcRect, &dstRect);
}
