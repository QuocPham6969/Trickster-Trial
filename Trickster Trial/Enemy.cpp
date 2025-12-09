#include "Enemy.h"
#include "TextureManager.h"
#include "LevelDesigner.h"
#include <iostream>

Enemy::Enemy() {}

Enemy::~Enemy()
{
    for (auto& pair : m_animations)
    {
        Animation& anim = pair.second;
        if (anim.texture)
        {
            SDL_DestroyTexture(anim.texture);
            anim.texture = nullptr;
        }
    }
}

SDL_Texture* Enemy::LoadAnimSheet(SDL_Renderer* renderer, const std::string& path)
{
    return TextureManager::Instance().LoadTexture(path, renderer);
}

// King Pig 

bool Enemy::InitKingPig(SDL_Renderer* renderer, const std::string& folderPath)
{
    m_animations.clear();

    m_frameWidth = 38;
    m_frameHeight = 28;

    auto loadAnim = [&](EnemyAnimState state, const std::string& fileName)
        {
            std::string fullPath = folderPath + "/" + fileName;
            SDL_Texture* tex = LoadAnimSheet(renderer, fullPath);
            if (!tex)
            {
                std::cout << "Failed to load King Pig sheet: " << fullPath << "\n";
                return false;
            }

            int texW = 0, texH = 0;
            SDL_QueryTexture(tex, nullptr, nullptr, &texW, &texH);

            Animation anim;
            anim.texture = tex;
            anim.frameCount = texW / m_frameWidth;
            m_animations[state] = anim;

            std::cout << "Loaded King Pig anim " << fullPath
                << " with " << anim.frameCount << " frames\n";

            return true;
        };

    if (!loadAnim(EnemyAnimState::Idle, "Idle (38x28).png")) return false;
    if (!loadAnim(EnemyAnimState::Run, "Run (38x28).png")) return false;
    if (!loadAnim(EnemyAnimState::Attack, "Attack (38x28).png")) return false;
    if (!loadAnim(EnemyAnimState::Hit, "Hit (38x28).png")) return false;
    if (!loadAnim(EnemyAnimState::Dead, "Dead (38x28).png")) return false;

    m_currentState = EnemyAnimState::Idle;
    m_currentFrame = 0;
    m_lastFrameTime = SDL_GetTicks();
    m_facingRight = false; // base art faces left
    m_maxHealth = 5;
    m_health = 5;
    m_isDead = false;
    m_isHit = false;
    m_lastHitAttackNumber = 0;
    m_hitEndTime = 0;

    return true;
}

// Minion Pig 

bool Enemy::InitPig(SDL_Renderer* renderer, const std::string& folderPath)
{
    m_animations.clear();

    m_frameWidth = 34;
    m_frameHeight = 28;

    auto loadAnim = [&](EnemyAnimState state, const std::string& fileName)
        {
            std::string fullPath = folderPath + "/" + fileName;
            SDL_Texture* tex = LoadAnimSheet(renderer, fullPath);
            if (!tex)
            {
                std::cout << "Failed to load Pig sheet: " << fullPath << "\n";
                return false;
            }

            int texW = 0, texH = 0;
            SDL_QueryTexture(tex, nullptr, nullptr, &texW, &texH);

            Animation anim;
            anim.texture = tex;
            anim.frameCount = texW / m_frameWidth;
            m_animations[state] = anim;

            std::cout << "Loaded Pig anim " << fullPath
                << " with " << anim.frameCount << " frames\n";

            return true;
        };

    if (!loadAnim(EnemyAnimState::Idle, "Idle (34x28).png")) return false;
    if (!loadAnim(EnemyAnimState::Run, "Run (34x28).png")) return false;
    if (!loadAnim(EnemyAnimState::Attack, "Attack (34x28).png")) return false;
    if (!loadAnim(EnemyAnimState::Hit, "Hit (34x28).png")) return false;
    if (!loadAnim(EnemyAnimState::Dead, "Dead (34x28).png")) return false;

    m_currentState = EnemyAnimState::Idle;
    m_currentFrame = 0;
    m_lastFrameTime = SDL_GetTicks();
    m_facingRight = false;
    m_maxHealth = 3;
    m_health = 3;
    m_isDead = false;
    m_isHit = false;
    m_lastHitAttackNumber = 0;
    m_hitEndTime = 0;

    return true;
}

// Shared behaviour

void Enemy::SetPosition(float x, float y)
{
    m_x = x;
    m_y = y;
}

void Enemy::SetState(EnemyAnimState newState)
{
    // Once dead, never leave Dead state
    if (m_isDead && newState != EnemyAnimState::Dead)
        return;

    // During hit-stun, only allow Hit or Dead
    if (m_isHit && newState != EnemyAnimState::Dead &&
        newState != EnemyAnimState::Hit)
        return;

    if (m_currentState == newState)
        return;

    m_currentState = newState;
    m_currentFrame = 0;
    m_lastFrameTime = SDL_GetTicks();
}

void Enemy::Update()
{
    Uint32 now = SDL_GetTicks();

    // End of hit-stun
    if (m_isHit && !m_isDead && now >= m_hitEndTime)
    {
        m_isHit = false;

        if (m_currentState == EnemyAnimState::Hit)
        {
            m_currentState = EnemyAnimState::Idle;
            m_currentFrame = 0;
            m_lastFrameTime = now;
        }
    }

	// Frame timing (for accurate per-frame animation)
    if (now - m_lastFrameTime < m_frameDurationMs)
        return;

    m_lastFrameTime += m_frameDurationMs;

    Animation& anim = m_animations[m_currentState];
    if (anim.frameCount <= 0 || !anim.texture)
        return;

    m_currentFrame++;

    if (m_currentState == EnemyAnimState::Dead)
    {
        // Clamp to last frame
        if (m_currentFrame >= anim.frameCount)
            m_currentFrame = anim.frameCount - 1;
    }
    else
    {
        if (m_currentState == EnemyAnimState::Hit &&
            m_currentFrame >= anim.frameCount)
        {
            // Stay on last hit frame while stunned
            m_currentFrame = anim.frameCount - 1;
        }
        else if (m_currentFrame >= anim.frameCount)
        {
            // Other states loop
            m_currentFrame = 0;
        }
    }
}

void Enemy::Render(SDL_Renderer* renderer)
{
    Animation& anim = m_animations[m_currentState];
    if (!anim.texture || anim.frameCount <= 0)
        return;

    SDL_Rect src;
    src.x = m_currentFrame * m_frameWidth;
    src.y = 0;
    src.w = m_frameWidth;
    src.h = m_frameHeight;

    SDL_Rect dst;
    dst.x = static_cast<int>(m_x);
    dst.y = static_cast<int>(m_y);
    dst.w = GetDrawWidth();
    dst.h = GetDrawHeight();

    // Base art faces LEFT; facingRight=true means flip to RIGHT
    SDL_RendererFlip flip = m_facingRight ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    SDL_RenderCopyEx(renderer, anim.texture, &src, &dst, 0.0, nullptr, flip);
}

void Enemy::MoveWithCollision(float dx, float dy, const LevelDesigner& level)
{
    int tileSize = LevelDesigner::TILE_SIZE_SCREEN;
    int width = GetDrawWidth();
    int height = GetDrawHeight();

    const float paddingX = 20.0f;
    const float paddingY = 10.0f;

    // Move X
    float newX = m_x + dx;

    float leftX = newX + paddingX;
    float rightX = newX + width - paddingX;
    float feetY = m_y + height - paddingY;

    int colLeft = static_cast<int>(leftX) / tileSize;
    int colRight = static_cast<int>(rightX) / tileSize;
    int rowFeet = static_cast<int>(feetY) / tileSize;

    bool blockedX = false;
    for (int c = colLeft; c <= colRight; ++c)
    {
        if (level.IsSolidCell(c, rowFeet))
        {
            blockedX = true;
            break;
        }
    }

    if (!blockedX)
        m_x = newX;

    // Move Y
    float newY = m_y + dy;

    leftX = m_x + paddingX;
    rightX = m_x + width - paddingX;
    feetY = newY + height - paddingY;

    colLeft = static_cast<int>(leftX) / tileSize;
    colRight = static_cast<int>(rightX) / tileSize;
    rowFeet = static_cast<int>(feetY) / tileSize;

    bool blockedY = false;
    for (int c = colLeft; c <= colRight; ++c)
    {
        if (level.IsSolidCell(c, rowFeet))
        {
            blockedY = true;
            break;
        }
    }

    if (!blockedY)
        m_y = newY;
}

void Enemy::ApplyDamage(int amount, unsigned int attackNumber)
{
    if (m_isDead)
        return;

    // Avoid multiple hits from the same hammer swing
    if (attackNumber == m_lastHitAttackNumber)
        return;

    m_lastHitAttackNumber = attackNumber;

    m_health -= amount;
    if (m_health <= 0)
    {
        m_health = 0;
        m_isDead = true;
        m_isHit = false;
        m_currentState = EnemyAnimState::Dead;
        m_currentFrame = 0;
        m_lastFrameTime = SDL_GetTicks();
        return;
    }

    // Still alive → hit-stun
    m_isHit = true;
    Uint32 now = SDL_GetTicks();
    m_hitEndTime = now + 250; // 0.25s stagger
    m_currentState = EnemyAnimState::Hit;
    m_currentFrame = 0;
    m_lastFrameTime = now;
}
