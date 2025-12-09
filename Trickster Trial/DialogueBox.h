#pragma once

#include <SDL.h>
#include <map>
#include <string>

// Simple state machine
enum class DialoguePhase
{
    None,
    ExclaimIn,
    ExclaimOut,
    AttackIn,
    AttackOut,
    Finished
};

class DialogueBox
{
public:
    DialogueBox();
    ~DialogueBox();

    bool Init(SDL_Renderer* renderer, const std::string& folderPath);

    // Call when the player first enters Level 2
    void Start();

    void Update();
    void Render(SDL_Renderer* renderer, float anchorX, float anchorY);

    // Playing = any active phase before Finished
    bool IsPlaying() const {
        return m_phase != DialoguePhase::None &&
            m_phase != DialoguePhase::Finished;
    }

    bool IsFinished() const { return m_phase == DialoguePhase::Finished; }

private:
    struct Animation
    {
        SDL_Texture* texture = nullptr;
        int frameCount = 0;
    };

    SDL_Texture* LoadAnimSheet(SDL_Renderer* renderer, const std::string& path);

    std::map<DialoguePhase, Animation> m_anims;

    DialoguePhase m_phase = DialoguePhase::None;

    int m_frameWidth = 34;
    int m_frameHeight = 16;
    int m_drawScale = 2;   // make it bigger on screen

    int GetDrawWidth() const { return m_frameWidth * m_drawScale; }
    int GetDrawHeight() const { return m_frameHeight * m_drawScale; }

    // Per-frame speed
    Uint32 m_frameDurationMs = 100;   // 0.1s per frame

    Uint32 m_lastFrameTime = 0;
    int m_currentFrame = 0;

    // Timing helpers for delays/holds
    Uint32 m_phaseStartTime = 0; // when this phase began
    Uint32 m_holdStartTime = 0; // when holding last frame
    bool m_startedFrames = false; // have we started animating frames yet?
    bool m_inHold = false; // currently holding the last frame

    void AdvancePhase();

    Uint32 GetPhaseStartDelay(DialoguePhase phase) const;
    Uint32 GetPhaseHoldDuration(DialoguePhase phase) const;
};
