#pragma once

#include <SDL.h>
#include <string>
#include <map>
#include "TextureManager.h"

// Simple 3-state door animation
enum class DoorAnimState
{
    Idle,
    Opening,
    Closing
};

class Door
{
public:
    Door();
    ~Door();

    bool Init(SDL_Renderer* renderer, const std::string& folderPath);

    void Update();
    void Render(SDL_Renderer* renderer);

    void SetState(DoorAnimState newState);
    void SetPosition(float x, float y);

    // Used for F-interaction distance checks
    SDL_FRect GetBounds() const;

    // "Tunnel" connection info
    void SetTravelTarget(int fromLevel, int toLevel, float targetX, float targetY);
    int GetFromLevel() const { return m_fromLevel; }
    int GetToLevel() const { return m_toLevel; }
    float GetTargetX() const { return m_targetX; }
    float GetTargetY() const { return m_targetY; }

private:
    struct Animation
    {
        SDL_Texture* texture = nullptr;
        int frameCount = 0;
    };

    SDL_Texture* LoadSheet(SDL_Renderer* renderer, const std::string& path);

    std::map<DoorAnimState, Animation> m_animations;
    DoorAnimState m_currentState = DoorAnimState::Idle;

    // Sprite sheet frame
    int m_frameWidth = 46;
    int m_frameHeight = 56;

    // On-screen size
    int m_drawScale = 2;
    int GetDrawWidth() const {return m_frameWidth * m_drawScale;}
    int GetDrawHeight() const {return m_frameHeight * m_drawScale;}

    // Animation timing
    Uint32 m_frameDurationMs = 100;   // 10 FPS
    Uint32 m_lastFrameTime = 0;
    int m_currentFrame = 0;

    // World position
    float m_x = 0.0f;
    float m_y = 0.0f;

    // Travel info (which levels this door connects)
    int m_fromLevel = 0;
    int m_toLevel = 1;
    float m_targetX = 0.0f;
    float m_targetY = 0.0f;
};
