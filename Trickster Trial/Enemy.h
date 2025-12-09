#pragma once

#include <SDL.h>
#include <map>
#include <string>

class LevelDesigner;

// Shared animation states for both minion + king pigs
enum class EnemyAnimState
{
    Idle,
    Run,
    Attack,
    Hit,
    Dead
};

class Enemy
{
public:
    Enemy();
    ~Enemy();

    // Boss init
    bool InitKingPig(SDL_Renderer* renderer, const std::string& folderPath);
    // Minion init
    bool InitPig(SDL_Renderer* renderer, const std::string& folderPath);

    void Update();
    void Render(SDL_Renderer* renderer);

    void SetState(EnemyAnimState newState);

    void SetPosition(float x, float y);
    float GetX() const { return m_x; }
    float GetY() const { return m_y; }

    int  GetWidth()  const { return GetDrawWidth(); }
    int  GetHeight() const { return GetDrawHeight(); }

    void SetFacingRight(bool right) { m_facingRight = right; }
    bool IsFacingRight() const { return m_facingRight; }

    void MoveWithCollision(float dx, float dy, const LevelDesigner& level);
    EnemyAnimState GetState() const { return m_currentState; }

    // Called by player when hit
    void ApplyDamage(int amount, unsigned int attackNumber);

    bool IsDead() const { return m_isDead; }
    bool IsStunned() const { return m_isHit; }

private:
    struct Animation
    {
        SDL_Texture* texture = nullptr;
        int frameCount = 0;
    };

    SDL_Texture* LoadAnimSheet(SDL_Renderer* renderer, const std::string& path);

    std::map<EnemyAnimState, Animation> m_animations;
    EnemyAnimState m_currentState = EnemyAnimState::Idle;

    // Sprite frame size (set by which Init* we use)
    int m_frameWidth = 38;
    int m_frameHeight = 28;

    // On-screen scale
    int GetDrawWidth()  const { return m_frameWidth * m_drawScale; }
    int GetDrawHeight() const { return m_frameHeight * m_drawScale; }
    int m_drawScale = 2;

    // Animation timing
    Uint32 m_frameDurationMs = 100;
    Uint32 m_lastFrameTime = 0;
    int m_currentFrame = 0;

    // World position
    float m_x = 0.0f;
    float m_y = 0.0f;

    bool m_facingRight = false;

    int m_health = 1;
    int m_maxHealth = 1;

    bool m_isDead = false;
    bool m_isHit = false;
    Uint32 m_hitEndTime = 0; // when stagger ends

    // Used so one hammer swing cannot hit multiple times
    unsigned int m_lastHitAttackNumber = 0;
};
