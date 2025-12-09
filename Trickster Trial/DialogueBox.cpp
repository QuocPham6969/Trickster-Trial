#include "DialogueBox.h"
#include "TextureManager.h"
#include <iostream>

DialogueBox::DialogueBox() {}

DialogueBox::~DialogueBox()
{
    for (auto& pair : m_anims)
    {
        Animation& anim = pair.second;
        if (anim.texture)
        {
            SDL_DestroyTexture(anim.texture);
            anim.texture = nullptr;
        }
    }
}

SDL_Texture* DialogueBox::LoadAnimSheet(SDL_Renderer* renderer, const std::string& path)
{
    return TextureManager::Instance().LoadTexture(path, renderer);
}

bool DialogueBox::Init(SDL_Renderer* renderer, const std::string& folderPath)
{
    auto loadAnim = [&](DialoguePhase phase, const std::string& fileName)
        {
            std::string fullPath = folderPath + "/" + fileName;
            SDL_Texture* tex = LoadAnimSheet(renderer, fullPath);
            if (!tex)
            {
                std::cout << "Failed to load dialogue sheet: " << fullPath << "\n";
                return false;
            }

            int texW = 0, texH = 0;
            SDL_QueryTexture(tex, nullptr, nullptr, &texW, &texH);

            Animation anim;
            anim.texture = tex;
            anim.frameCount = texW / m_frameWidth;
            m_anims[phase] = anim;

            std::cout << "Loaded dialogue anim " << fullPath
                << " with " << anim.frameCount << " frames\n";

            return true;
        };

    if (!loadAnim(DialoguePhase::ExclaimIn, "!!! In (24x8).png")) return false;
    if (!loadAnim(DialoguePhase::ExclaimOut, "!!! Out (24x8).png")) return false;
    if (!loadAnim(DialoguePhase::AttackIn, "Attack In (24x8).png")) return false;
    if (!loadAnim(DialoguePhase::AttackOut, "Attack Out (24x8).png")) return false;

    m_phase = DialoguePhase::None;
    m_currentFrame = 0;
    m_lastFrameTime = SDL_GetTicks();
    m_phaseStartTime = m_lastFrameTime;
    m_startedFrames = false;
    m_inHold = false;

    return true;
}

void DialogueBox::Start()
{
    m_phase = DialoguePhase::ExclaimIn;
    m_currentFrame = 0;
    Uint32 now = SDL_GetTicks();
    m_lastFrameTime = now;
    m_phaseStartTime = now;
    m_startedFrames = false;  // wait initial delay
    m_inHold = false;
}

// Delay BEFORE starting frame animation in each phase
Uint32 DialogueBox::GetPhaseStartDelay(DialoguePhase phase) const
{
    switch (phase)
    {
    case DialoguePhase::ExclaimIn: return 500;
    case DialoguePhase::AttackIn: return 500;
    default: return 0;
    }
}

// How long to HOLD the LAST frame after the animation finishes
Uint32 DialogueBox::GetPhaseHoldDuration(DialoguePhase phase) const
{
    switch (phase)
    {
    case DialoguePhase::ExclaimIn: return 1000;
    case DialoguePhase::AttackIn: return 1000;
    default: return 0;
    }
}

void DialogueBox::AdvancePhase()
{
    switch (m_phase)
    {
    case DialoguePhase::ExclaimIn: m_phase = DialoguePhase::ExclaimOut; break;
    case DialoguePhase::ExclaimOut: m_phase = DialoguePhase::AttackIn; break;
    case DialoguePhase::AttackIn: m_phase = DialoguePhase::AttackOut; break;
    case DialoguePhase::AttackOut: m_phase = DialoguePhase::Finished; break;
    default: m_phase = DialoguePhase::Finished; break;
    }

    m_currentFrame = 0;
    Uint32 now = SDL_GetTicks();
    m_lastFrameTime = now;
    m_phaseStartTime = now;
    m_startedFrames = false;
    m_inHold = false;
}

void DialogueBox::Update()
{
    if (!IsPlaying())
        return;

    Uint32 now = SDL_GetTicks();

    // Wait initial delay for this phase before animating frames
    if (!m_startedFrames)
    {
        Uint32 startDelay = GetPhaseStartDelay(m_phase);
        if (now - m_phaseStartTime < startDelay)
            return;

        // delay finished - begin animating frames
        m_startedFrames = true;
        m_lastFrameTime = now;
    }

    // If holding on the last frame, wait until hold duration ends
    if (m_inHold)
    {
        Uint32 holdDuration = GetPhaseHoldDuration(m_phase);
        if (now - m_holdStartTime >= holdDuration)
        {
            // move to next phase
            AdvancePhase();
        }
        return;
    }

    // Normal frame-by-frame animation
    if (now - m_lastFrameTime < m_frameDurationMs)
        return;

    m_lastFrameTime = now;

    Animation& anim = m_anims[m_phase];
    if (anim.frameCount <= 0 || !anim.texture)
        return;

    m_currentFrame++;

    if (m_currentFrame >= anim.frameCount)
    {
        // Finished this phase's frames
        Uint32 holdDuration = GetPhaseHoldDuration(m_phase);
        if (holdDuration > 0)
        {
            // Clamp to last frame and start hold
            m_currentFrame = anim.frameCount - 1;
            m_inHold = true;
            m_holdStartTime = now;
        }
        else
        {
            // No hold, advance immediately
            AdvancePhase();
        }
    }
}

void DialogueBox::Render(SDL_Renderer* renderer, float anchorX, float anchorY)
{
    if (!IsPlaying())
        return;

    Animation& anim = m_anims[m_phase];
    if (!anim.texture || anim.frameCount <= 0)
        return;

    SDL_Rect src;
    src.x = m_currentFrame * m_frameWidth;
    src.y = 0;
    src.w = m_frameWidth;
    src.h = m_frameHeight;

    SDL_Rect dst;
    dst.w = GetDrawWidth();
    dst.h = GetDrawHeight();

    // Hover above KING PIG's head
    dst.x = static_cast<int>(anchorX + 5);
    dst.y = static_cast<int>(anchorY - 20);

    SDL_RenderCopy(renderer, anim.texture, &src, &dst);
}
