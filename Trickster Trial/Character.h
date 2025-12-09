#pragma once

#include <SDL.h>
#include <map>
#include <string>

class LevelDesigner; // forward declaration

// All possible animation states for the player
enum class AnimState
{
    Idle,
    Run,
    Jump,
    Fall,
    Ground,
    Attack,
    Hit,
    Dead,
    DoorIn,
    DoorOut
};

class Character
{
public:
    Character();
    ~Character();

    bool Init(SDL_Renderer* renderer);

    // per-frame logic & drawing
    void Update();
    void Render(SDL_Renderer* renderer);

    // state changes (Run, Attack, Hit, etc.)
    void SetState(AnimState newState);

    // pos / movement
    void SetPosition(float newX, float newY);
    void MoveWithCollision(float dx, float dy, const LevelDesigner& level);

    float GetX() const { return m_x; }
    float GetY() const { return m_y; }
    int   GetWidth()  const { return GetDrawWidth(); }
    int   GetHeight() const { return GetDrawHeight(); }

    // Attack info (used by Main for hit detection)
    bool         IsAttacking()     const { return m_isAttacking; }
    unsigned int GetAttackNumber() const { return m_attackNumber; }
    SDL_FRect    GetAttackHitBox() const;

    // Health/death
    int  GetHealth()    const { return m_health; }
    int  GetMaxHealth() const { return m_maxHealth; }
    bool IsDead()       const { return m_isDead; }

    // Called when enemies hit the player
    void ApplyDamage(int amount);

private:
    // Simple animation data for a whole sprite sheet
    struct Animation
    {
        SDL_Texture* texture = nullptr;
        int frameCount = 0;
    };

    SDL_Texture* LoadAnimSheet(SDL_Renderer* renderer, const std::string& path);

    std::map<AnimState, Animation> m_animations;
    AnimState m_currentState = AnimState::Idle;

    // Sprite sheet frame size
    int m_frameWidth = 78;
    int m_frameHeight = 58;

    // Draw scale to enlarge the sprite on screen
    int GetDrawWidth()  const { return m_frameWidth * m_drawScale; }
    int GetDrawHeight() const { return m_frameHeight * m_drawScale; }

    int    m_drawScale = 2;
    Uint32 m_frameDurationMs = 100;   
    Uint32 m_lastFrameTime = 0;
    int    m_currentFrame = 0;

    // World position
    float m_x = 100.0f;
    float m_y = 100.0f;

    bool m_facingRight = true;

    // Attack tracking (used to avoid multi-hits per swing)
    bool         m_isAttacking = false;
    bool         m_attackQueued = false;
    unsigned int m_attackNumber = 0;

    // Health & invincibility
    int    m_health = 3;
    int    m_maxHealth = 3;
    bool   m_isDead = false;
    bool   m_isInvincible = false;
    Uint32 m_invincibleEndTime = 0;

    // Hit-stun when player gets hit
    bool   m_isHit = false;
    Uint32 m_hitEndTime = 0;
};
