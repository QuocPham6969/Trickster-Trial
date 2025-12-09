#include "Character.h"
#include "TextureManager.h"
#include "LevelDesigner.h"
#include <iostream>

Character::Character() {}

Character::~Character()
{
    // Destroy all loaded animation textures
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

bool Character::Init(SDL_Renderer* renderer)
{
    // Helper lambda to load one animation sheet
    auto loadAnim = [&](AnimState state, const std::string& fileName)
        {
            std::string fullPath = "assets/anim/Human/" + fileName;
            SDL_Texture* tex = LoadAnimSheet(renderer, fullPath);
            if (!tex)
            {
                std::cout << "Failed to load animation sheet: " << fullPath << "\n";
                return false;
            }

            int texW = 0, texH = 0;
            SDL_QueryTexture(tex, nullptr, nullptr, &texW, &texH);

            Animation anim;
            anim.texture = tex;
            anim.frameCount = texW / m_frameWidth;
            m_animations[state] = anim;

            std::cout << "Loaded " << fullPath
                << " with " << anim.frameCount << " frames\n";

            return true;
        };

    // Anims
    if (!loadAnim(AnimState::Idle,"Idle (78x58).png")) return false;
    if (!loadAnim(AnimState::Run,"Run (78x58).png")) return false;
    if (!loadAnim(AnimState::Jump,"Jump (78x58).png")) return false;
    if (!loadAnim(AnimState::Fall,"Fall (78x58).png")) return false;
    if (!loadAnim(AnimState::Ground,"Ground (78x58).png")) return false;
    if (!loadAnim(AnimState::Attack,"Attack (78x58).png")) return false;
    if (!loadAnim(AnimState::Hit,"Hit (78x58).png")) return false;
    if (!loadAnim(AnimState::Dead,"Dead (78x58).png")) return false;
    if (!loadAnim(AnimState::DoorIn,"Door In (78x58).png")) return false;
    if (!loadAnim(AnimState::DoorOut,"Door Out (78x58).png")) return false;

    m_currentState = AnimState::Idle;
    m_currentFrame = 0;
    m_lastFrameTime = SDL_GetTicks();

    return true;
}

SDL_Texture* Character::LoadAnimSheet(SDL_Renderer* renderer, const std::string& path)
{
    return TextureManager::Instance().LoadTexture(path, renderer);
}

void Character::Update()
{
    Uint32 now = SDL_GetTicks();

    // Hit stun/invincibility timers
    if (m_isHit && now >= m_hitEndTime)
    {
        m_isHit = false;

        if (m_currentState == AnimState::Hit)
        {
            // Return to idle when hit-stun ends
            m_currentState = AnimState::Idle;
            m_currentFrame = 0;
            m_lastFrameTime = now;
        }
    }

    if (m_isInvincible && now >= m_invincibleEndTime)
    {
        m_isInvincible = false;
    }

	// Frame timing (for accurate per-frame speed)
    if (now - m_lastFrameTime < m_frameDurationMs)
        return;

    m_lastFrameTime += m_frameDurationMs;

    Animation& anim = m_animations[m_currentState];
    if (anim.frameCount <= 0 || !anim.texture)
        return;

    m_currentFrame++;

    // Animation loop rules per state
    if (m_currentState == AnimState::Attack)
    {
        if (m_currentFrame >= anim.frameCount)
        {
            // Attack finished
            if (m_attackQueued)
            {
                // Start chained attack
                m_attackQueued = false;
                m_isAttacking = true;
                m_currentState = AnimState::Attack;
                m_currentFrame = 0;
                m_lastFrameTime = now;  // avoid instant skip
            }
            else
            {
                // Return to idle
                m_isAttacking = false;
                m_currentState = AnimState::Idle;
                m_currentFrame = 0;
            }
        }
    }
    else if (m_currentState == AnimState::Hit)
    {
        // Clamp Hit to last frame (no loop)
        if (m_currentFrame >= anim.frameCount)
            m_currentFrame = anim.frameCount - 1;
    }
    else if (m_currentState == AnimState::Dead)
    {
        // Clamp Dead to last frame
        if (m_currentFrame >= anim.frameCount)
            m_currentFrame = anim.frameCount - 1;
    }
    else
    {
        // Other animations loop
        if (m_currentFrame >= anim.frameCount)
            m_currentFrame = 0;
    }
}

void Character::Render(SDL_Renderer* renderer)
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

	SDL_RendererFlip flip = m_facingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL; // base art faces right
    SDL_RenderCopyEx(renderer, anim.texture, &src, &dst, 0.0, nullptr, flip);
}

void Character::SetState(AnimState newState)
{
    // special handling: attack (queueable)
    if (newState == AnimState::Attack)
    {
        if (m_isDead)   // dead can't attack
            return;

		// If already attacking, queue another swing, avoid double-trigger/click
        if (m_isAttacking)
        {
            if (!m_attackQueued)
                m_attackQueued = true;
            return;
        }

        m_isAttacking = true;
        ++m_attackNumber;               // unique ID per swing
        m_currentState = AnimState::Attack;
        m_currentFrame = 0;
        m_lastFrameTime = SDL_GetTicks();
        return;
    }

    // Hit/Dead overrides everything immediately
    if (newState == AnimState::Hit || newState == AnimState::Dead)
    {
        m_isAttacking = false;
        m_attackQueued = false;

        if (newState == AnimState::Dead)
            m_isDead = true;

        m_currentState = newState;
        m_currentFrame = 0;
        m_lastFrameTime = SDL_GetTicks();
        return;
    }

    // While attacking, in hit-stun, or dead, ignore normal changes
    if (m_isAttacking || m_isHit || m_isDead)
        return;

    if (m_currentState == newState)
        return;

    m_currentState = newState;
    m_currentFrame = 0;
    m_lastFrameTime = SDL_GetTicks();
}

void Character::SetPosition(float newX, float newY) // position for the character
{
    m_x = newX;
    m_y = newY;
}

void Character::MoveWithCollision(float dx, float dy, const LevelDesigner& level)
{
    int tileSize = LevelDesigner::TILE_SIZE_SCREEN;
    int width = GetDrawWidth();
    int height = GetDrawHeight();

    // Small padding so hammer/hat can overlap walls slightly
    const float paddingX = 60.0f;
    const float paddingY = 30.0f;

    // Move X first
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

    // Update facing direction
    if (dx > 0.0f) m_facingRight = true;
    if (dx < 0.0f) m_facingRight = false;

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

// Hammer hit box in front of the character (for combat)
SDL_FRect Character::GetAttackHitBox() const
{
    SDL_FRect box{};

    float width = GetDrawWidth();
    float height = GetDrawHeight();

    box.w = width * 0.6f;
    box.h = height * 0.6f;
    box.y = m_y + height * 0.2f;

    if (m_facingRight)
    {
        box.x = m_x + width * 0.5f; // right side
    }
    else
    {
        box.x = m_x - box.w * 0.5f; // left side
    }

    return box;
}

// Called when an enemy hits the player
void Character::ApplyDamage(int amount)
{
    if (m_isDead)
        return;

    Uint32 now = SDL_GetTicks();

    // Respect invincibility window
    if (m_isInvincible && now < m_invincibleEndTime)
        return;

    m_health -= amount;
    if (m_health <= 0)
    {
        // Death
        m_health = 0;
        m_isDead = true;
        m_isAttacking = false;
        m_attackQueued = false;
        m_isHit = false;
        m_isInvincible = false;

        m_currentState = AnimState::Dead;
        m_currentFrame = 0;
        m_lastFrameTime = now;
        return;
    }

    // Still alive - go into Hit + short stun + invincibility
    m_isAttacking = false;
    m_attackQueued = false;

    m_isHit = true;
    m_hitEndTime = now + 250; // 0.25s stun
    m_isInvincible = true;
    m_invincibleEndTime = now + 1000; // 1s i-frames

    m_currentState = AnimState::Hit;
    m_currentFrame = 0;
    m_lastFrameTime = now;
}
