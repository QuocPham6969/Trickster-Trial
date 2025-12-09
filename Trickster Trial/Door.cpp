#include "Door.h"
#include <iostream>

Door::Door() {}

Door::~Door()
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

SDL_Texture* Door::LoadSheet(SDL_Renderer* renderer, const std::string& path)
{
    return TextureManager::Instance().LoadTexture(path, renderer);
}

bool Door::Init(SDL_Renderer* renderer, const std::string& folderPath)
{
    auto loadAnim = [&](DoorAnimState state, const std::string& fileName)
        {
            std::string fullPath = folderPath + "/" + fileName;
            SDL_Texture* tex = LoadSheet(renderer, fullPath);
            if (!tex)
            {
                std::cout << "Failed to load door sheet: " << fullPath << "\n";
                return false;
            }

            int texW = 0, texH = 0;
            SDL_QueryTexture(tex, nullptr, nullptr, &texW, &texH);

            Animation anim;
            anim.texture = tex;
            anim.frameCount = texW / m_frameWidth;
            m_animations[state] = anim;

            std::cout << "Loaded door anim " << fullPath
                << " with " << anim.frameCount << " frames\n";

            return true;
        };

    if (!loadAnim(DoorAnimState::Idle, "Idle.png")) return false;
    if (!loadAnim(DoorAnimState::Opening, "Opening (46x56).png")) return false;
    if (!loadAnim(DoorAnimState::Closing, "Closing (46x56).png")) return false;

    m_currentState = DoorAnimState::Idle;
    m_currentFrame = 0;
    m_lastFrameTime = SDL_GetTicks();

    return true;
}

void Door::SetPosition(float x, float y)
{
    m_x = x;
    m_y = y;
}

SDL_FRect Door::GetBounds() const
{
    SDL_FRect r;
    r.x = m_x;
    r.y = m_y;
    r.w = static_cast<float>(GetDrawWidth());
    r.h = static_cast<float>(GetDrawHeight());
    return r;
}

void Door::SetTravelTarget(int fromLevel, int toLevel, float targetX, float targetY)
{
    m_fromLevel = fromLevel;
    m_toLevel = toLevel;
    m_targetX = targetX;
    m_targetY = targetY;
}

void Door::SetState(DoorAnimState newState)
{
    if (m_currentState == newState)
        return;

    m_currentState = newState;
    m_currentFrame = 0;
    m_lastFrameTime = SDL_GetTicks();
}

void Door::Update()
{
    Uint32 now = SDL_GetTicks();
    if (now - m_lastFrameTime < m_frameDurationMs)
        return;

    m_lastFrameTime += m_frameDurationMs;

    Animation& anim = m_animations[m_currentState];
    if (anim.frameCount <= 0 || !anim.texture)
        return;

    m_currentFrame++;

    if (m_currentState == DoorAnimState::Opening || m_currentState == DoorAnimState::Closing)
    {
        // Opening/Closing play once then return to Idle
        if (m_currentFrame >= anim.frameCount)
        {
            m_currentState = DoorAnimState::Idle;
            m_currentFrame = 0;
        }
    }
    else
    {
        // Idle loops
        if (m_currentFrame >= anim.frameCount)
            m_currentFrame = 0;
    }
}

void Door::Render(SDL_Renderer* renderer)
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

    SDL_RenderCopy(renderer, anim.texture, &src, &dst);
}
