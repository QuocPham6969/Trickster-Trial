#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <string>
#include <cmath>
#include <vector>

#include "TextureManager.h"
#include "LevelDesigner.h"
#include "Character.h"
#include "Door.h"
#include "Enemy.h"
#include "DialogueBox.h"

// Teleport state when using doors
enum class DoorTravelState
{
    None,
    GoingIn,
    ComingOut
};

int main(int argc, char* argv[])
{
    // SDL/window/renderer setup

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    {
        std::cout << "SDL_Init Error: " << SDL_GetError() << "\n";
        return 1;
    }

    int imgFlags = IMG_INIT_PNG;
    if ((IMG_Init(imgFlags) & imgFlags) != imgFlags)
    {
        std::cout << "IMG_Init Error: " << IMG_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Trickster's Trial",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_SHOWN);

    if (!window)
    {
        std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << "\n";
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!renderer)
    {
        std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Level tiles

    LevelDesigner levelDesigner;
    if (!levelDesigner.Init(renderer))
    {
        std::cout << "Failed to init LevelDesigner\n";
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Disable paint mode by default (F1/F2 to edit levels)
    levelDesigner.paintingEnabled = false;

    // Player

    Character player;
    if (!player.Init(renderer))
    {
        std::cout << "Failed to init Character\n";
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Start in Level 1
    player.SetPosition(5.0f, 280.0f);
    int playerLevelIndex = 0; // 0 = level1, 1 = level2

    // Doors (tunnel between levels)

    Door doorLevel0To1;
    Door doorLevel1To0;
    std::string doorFolder = "assets/anim/Door";

    if (!doorLevel0To1.Init(renderer,doorFolder) ||
        !doorLevel1To0.Init(renderer,doorFolder))
    {
        std::cout << "Failed to init Door(s)\n";
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Both doors share the same on-screen position in their levels
    doorLevel0To1.SetPosition(1150, 260.0f); // Level 0 door → Level 1
    doorLevel1To0.SetPosition(50, 260.0f); // Level 1 door → Level 0

    // Where the player appears after using each door
    doorLevel0To1.SetTravelTarget(
        0, 1,
        64.0f * 0.9f,   // spawn in level 2
        64.0f * 4.5f);

    doorLevel1To0.SetTravelTarget(
        1, 0,
        64.0f * 17.5f,  // spawn in level 1
        64.0f * 4.5f);

    // Enemies 

    Enemy kingPig;
    if (!kingPig.InitKingPig(renderer, "assets/anim/King Pig"))
    {
        std::cout << "Failed to init King Pig\n";
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // King is in Level 2 (right side)
    kingPig.SetPosition(1170.0f, 280.0f);

    // Minion pigs (all live in Level 2)
    std::vector<Enemy> minionPigs;
    const int MINION_COUNT = 3;
    minionPigs.resize(MINION_COUNT);

    for (int i = 0; i < MINION_COUNT; ++i)
    {
        if (!minionPigs[i].InitPig(renderer, "assets/anim/Pig"))
        {
            std::cout << "Failed to init Minion Pig " << i << "\n";
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            IMG_Quit();
            SDL_Quit();
            return 1;
        }
    }

    // Initial positions for pigs
    minionPigs[0].SetPosition(600.0f, 20.0f);   
    minionPigs[1].SetPosition(600.0f, 610.0f);   
    minionPigs[2].SetPosition(1000.0f, 280.0f);  

    // UI:Life bar

    SDL_Texture* liveBarTex = TextureManager::Instance().LoadTexture("assets/anim/Live and Coins/Live Bar.png",renderer);

    if (!liveBarTex)
    {
        std::cout << "Failed to load Live Bar UI\n";
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    const int LIVE_BAR_FULL_W = 66;
    const int LIVE_BAR_H = 34;
    const int HEART_W = LIVE_BAR_FULL_W / 3; // 3 hearts

    // King Pig dialogue

    DialogueBox kingPigDialogue;
    if (!kingPigDialogue.Init(renderer, "assets/anim/Dialogue Boxes"))
    {
        std::cout << "Failed to init King Pig dialogue\n";
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    bool kingPigDialogueStarted = false;
    bool minionChaseUnlocked = false; // minions wait for dialogue
    Uint32 minionChaseUnlockStart = 0;     // when we started 0.5s timer

    // King Pig wake-up when minion dies or player gets close
    bool kingPigAwake = false;
    float kingPigWakeRadius = 220.0f; // radius from king center

    // Teleport state

    bool running = true;
    SDL_Event e;
    Uint32 lastTicks = SDL_GetTicks();
    DoorTravelState travelState = DoorTravelState::None;
    Uint32 travelStartTime = 0;
    Door* activeDoor = nullptr;

    bool fWasDown = false; // for detecting fresh F press

    // Helper: check if player is within radius of a door
    auto IsPlayerNearDoor = [&](const Character& p, const Door& d) -> bool
        {
            SDL_FRect doorRect = d.GetBounds();

            float px = p.GetX() + p.GetWidth() * 0.5f;
            float py = p.GetY() + p.GetHeight() * 0.5f;

            float dx = px - (doorRect.x + doorRect.w * 0.5f);
            float dy = py - (doorRect.y + doorRect.h * 0.5f);

            float distSq = dx * dx + dy * dy;
            float maxDist = 80.0f;
            float maxDistSq = maxDist * maxDist;

            return distSq <= maxDistSq;
        };

    // Helper: simple AABB overlap
    auto RectsOverlap = [](const SDL_FRect& a, const SDL_FRect& b) -> bool
        {
            return !(a.x + a.w <= b.x ||
                b.x + b.w <= a.x ||
                a.y + a.h <= b.y ||
                b.y + b.h <= a.y);
        };

    // GAME LOOP

    while (running)
    {
        // Delta time 
        Uint32 now = SDL_GetTicks();
        float dt = (now - lastTicks) / 1000.0f;
        lastTicks = now;

        // Events 
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                running = false;

            // Left mouse button triggers attack
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
            {
                player.SetState(AnimState::Attack);
            }

            levelDesigner.HandleEvent(e);
        }

        // Update door animation frames
        doorLevel0To1.Update();
        doorLevel1To0.Update();

        // Keyboard state each frame
        const Uint8* keystate = SDL_GetKeyboardState(nullptr);
        float speed = 180.0f; // player move speed (pixels/s)

        float dx = 0.0f;
        float dy = 0.0f;

        //  PLAYER CONTROL/AI/COMBAT (when NOT teleporting)
        if (travelState == DoorTravelState::None)
        {
            // Player movement & door interaction
            if (levelDesigner.GetActiveLevel() == playerLevelIndex)
            {
                if (!player.IsDead())
                {
                    // WASD movement
                    if (keystate[SDL_SCANCODE_A] || keystate[SDL_SCANCODE_LEFT])
                        dx -= speed * dt;
                    if (keystate[SDL_SCANCODE_D] || keystate[SDL_SCANCODE_RIGHT])
                        dx += speed * dt;
                    if (keystate[SDL_SCANCODE_W] || keystate[SDL_SCANCODE_UP])
                        dy -= speed * dt;
                    if (keystate[SDL_SCANCODE_S] || keystate[SDL_SCANCODE_DOWN])
                        dy += speed * dt;

                    if (dx != 0.0f || dy != 0.0f)
                    {
                        player.MoveWithCollision(dx, dy, levelDesigner);
                        player.SetState(AnimState::Run);
                    }
                    else
                    {
                        player.SetState(AnimState::Idle);
                    }

                    // Door interaction (press F near door)
                    bool fDown = keystate[SDL_SCANCODE_F];

                    if (fDown && !fWasDown)
                    {
                        Door* candidateDoor = nullptr;
                        if (playerLevelIndex == 0)
                            candidateDoor = &doorLevel0To1;
                        else if (playerLevelIndex == 1)
                            candidateDoor = &doorLevel1To0;

                        if (candidateDoor && IsPlayerNearDoor(player, *candidateDoor))
                        {
                            activeDoor = candidateDoor;
                            travelState = DoorTravelState::GoingIn;
                            travelStartTime = now;

                            activeDoor->SetState(DoorAnimState::Opening);
                            player.SetState(AnimState::DoorIn);
                        }
                    }

                    fWasDown = fDown;
                }
                else
                {
                    // Dead: no movement / no door usage
                    dx = dy = 0.0f;
                    fWasDown = keystate[SDL_SCANCODE_F];
                }
            }

            // Dialogue + AI timing

            kingPigDialogue.Update();

            // Minions start chasing 0.5s AFTER dialogue finishes
            if (!minionChaseUnlocked && kingPigDialogueStarted && kingPigDialogue.IsFinished())
            {
                if (minionChaseUnlockStart == 0)
                    minionChaseUnlockStart = now;
                else if (now - minionChaseUnlockStart >= 500) // 0.5 seconds
                    minionChaseUnlocked = true;
            }

            // Enemy AI only when in Level 2
            if (levelDesigner.GetActiveLevel() == 1)
            {
                // Minion pigs
                for (size_t i = 0; i < minionPigs.size(); ++i)
                {
                    Enemy& pig = minionPigs[i];

                    if (!minionChaseUnlocked)
                    {
                        if (!pig.IsDead())
                            pig.SetState(EnemyAnimState::Idle);
                        continue;
                    }

                    // No movement while stunned or dead
                    if (pig.IsDead() || pig.IsStunned())
                        continue;

                    // Centers
                    float playerCenterX = player.GetX() + player.GetWidth() * 0.5f;
                    float playerCenterY = player.GetY() + player.GetHeight() * 0.5f;
                    float pigCenterX = pig.GetX() + pig.GetWidth() * 0.5f;
                    float pigCenterY = pig.GetY() + pig.GetHeight() * 0.5f;

                    // Always face the player
                    pig.SetFacingRight(playerCenterX > pigCenterX);

                    // Melee attack range 
                    const float ATTACK_RANGE = 30.0f; // distance from pig center
                    const float VERT_TOLERANCE = 16.0f; // vertical leniency

                    float dxAttack = playerCenterX - pigCenterX;
                    float dyAttack = playerCenterY - pigCenterY;
                    float distSqAttack = dxAttack * dxAttack + dyAttack * dyAttack;

                    if (distSqAttack <= ATTACK_RANGE * ATTACK_RANGE &&
                        std::abs(dyAttack) <= VERT_TOLERANCE &&
                        !player.IsDead())
                    {
                        // In melee range = attack animation + damage
                        pig.SetState(EnemyAnimState::Attack);
                        player.ApplyDamage(1);
                        continue; // skip movement this frame
                    }

                    // Surrounding behaviour (offset per pig index)
                    float offsetX = 0.0f;
                    float offsetY = 0.0f;

                    if (i == 0) offsetX = -40.0f; // left
                    if (i == 1) offsetX = 40.0f; // right
                    if (i == 2) offsetY = -40.0f; // top
                    if (i == 3) offsetY = 40.0f; // bottom

                    float targetX = playerCenterX + offsetX;
                    float targetY = playerCenterY + offsetY;

                    float dirX = targetX - pigCenterX;
                    float dirY = targetY - pigCenterY;
                    float lenSq = dirX * dirX + dirY * dirY;

                    if (lenSq > 1.0f)
                    {
                        float len = std::sqrt(lenSq);
                        dirX /= len;
                        dirY /= len;

                        float moveX = dirX * 120.0f * dt;
                        float moveY = dirY * 120.0f * dt;

                        pig.SetState(EnemyAnimState::Run);
                        pig.MoveWithCollision(moveX, moveY, levelDesigner);
                    }
                    else
                    {
                        pig.SetState(EnemyAnimState::Idle);
                    }
                }

                // King Pig

                // Check if any minion is dead
                bool anyMinionDead = false;
                for (const Enemy& pig : minionPigs)
                {
                    if (pig.GetState() == EnemyAnimState::Dead)
                    {
                        anyMinionDead = true;
                        break;
                    }
                }

                // Player distance from king
                float kingCenterX = kingPig.GetX() + kingPig.GetWidth() * 0.5f;
                float kingCenterY = kingPig.GetY() + kingPig.GetHeight() * 0.5f;
                float playerCenterX = player.GetX() + player.GetWidth() * 0.5f;
                float playerCenterY = player.GetY() + player.GetHeight() * 0.5f;

                float dxKing = playerCenterX - kingCenterX;
                float dyKing = playerCenterY - kingCenterY;
                float distSqKing = dxKing * dxKing + dyKing * dyKing;

                if (!kingPigAwake)
                {
                    if (anyMinionDead ||
                        distSqKing <= kingPigWakeRadius * kingPigWakeRadius)
                    {
                        kingPigAwake = true;
                    }
                }

                // Face toward the player
                kingPig.SetFacingRight(playerCenterX > kingCenterX);

                // King melee attack range
                const float KING_ATTACK_RANGE = 50.0f;
                const float KING_VERT_TOL = 18.0f;

                float dxAttackK = playerCenterX - kingCenterX;
                float dyAttackK = playerCenterY - kingCenterY;
                float distSqAttackK = dxAttackK * dxAttackK + dyAttackK * dyAttackK;

                if (kingPigAwake && !kingPig.IsDead() && !kingPig.IsStunned() && distSqAttackK <= KING_ATTACK_RANGE * KING_ATTACK_RANGE && std::abs(dyAttackK) <= KING_VERT_TOL &&
                    !player.IsDead())
                {
                    kingPig.SetState(EnemyAnimState::Attack);
                    player.ApplyDamage(1);
                }
                else if (kingPigAwake && !kingPig.IsDead() && !kingPig.IsStunned())
                {
                    // Chase the player
                    if (distSqKing > 1.0f)
                    {
                        float len = std::sqrt(distSqKing);
                        dxKing /= len;
                        dyKing /= len;

                        const float KING_SPEED = 95.0f;
                        float kdx = dxKing * KING_SPEED * dt;
                        float kdy = dyKing * KING_SPEED * dt;

                        kingPig.SetState(EnemyAnimState::Run);
                        kingPig.MoveWithCollision(kdx, kdy, levelDesigner);
                    }
                    else
                    {
                        kingPig.SetState(EnemyAnimState::Idle);
                    }
                }
                else if (!kingPig.IsDead())
                {
                    kingPig.SetState(EnemyAnimState::Idle);
                }
            }
            else
            {
                // Player not in Level 2 → keep pigs idle
                for (Enemy& pig : minionPigs)
                    pig.SetState(EnemyAnimState::Idle);
                kingPig.SetState(EnemyAnimState::Idle);
            }

            // Player attack vs enemies 

            if (levelDesigner.GetActiveLevel() == 1 && player.IsAttacking())
            {
                SDL_FRect hitBox = player.GetAttackHitBox();
                unsigned int atkId = player.GetAttackNumber();

                // Minions
                for (Enemy& pig : minionPigs)
                {
                    if (pig.IsDead())
                        continue;

                    SDL_FRect pigRect{
                        pig.GetX(),
                        pig.GetY(),
                        static_cast<float>(pig.GetWidth()),
                        static_cast<float>(pig.GetHeight())
                    };

                    if (RectsOverlap(hitBox, pigRect))
                    {
                        pig.ApplyDamage(1, atkId); // 3 HP total
                    }
                }

                // King
                if (!kingPig.IsDead())
                {
                    SDL_FRect kingRect{
                        kingPig.GetX(),
                        kingPig.GetY(),
                        static_cast<float>(kingPig.GetWidth()),
                        static_cast<float>(kingPig.GetHeight())
                    };

                    if (RectsOverlap(hitBox, kingRect))
                    {
                        kingPig.ApplyDamage(1, atkId); // 5 HP total
                    }
                }
            }

            // Update animation frames 
            player.Update();
            kingPig.Update();
            for (Enemy& pig : minionPigs)
                pig.Update();
        }
        else
        {
            
            //  TELEPORTING THROUGH DOOR (no control)
            const Uint32 phaseDuration = 600; // ms per door phase
            Uint32 elapsed = now - travelStartTime;

            if (travelState == DoorTravelState::GoingIn)
            {
                if (elapsed >= phaseDuration && activeDoor)
                {
                    // Move to target level/position
                    int newLevel = activeDoor->GetToLevel();
                    playerLevelIndex = newLevel;
                    levelDesigner.SetActiveLevel(newLevel);

                    player.SetPosition(activeDoor->GetTargetX(),
                        activeDoor->GetTargetY());
                    player.SetState(AnimState::DoorOut);

                    // Start King dialogue the first time we enter Level 2
                    if (newLevel == 1 && !kingPigDialogueStarted)
                    {
                        kingPigDialogue.Start();
                        kingPigDialogueStarted = true;
                    }

                    // Switch to the opposite door
                    if (activeDoor == &doorLevel0To1)
                        activeDoor = &doorLevel1To0;
                    else
                        activeDoor = &doorLevel0To1;

                    activeDoor->SetState(DoorAnimState::Opening);

                    travelState = DoorTravelState::ComingOut;
                    travelStartTime = now;
                }
            }
            else if (travelState == DoorTravelState::ComingOut)
            {
                if (elapsed >= phaseDuration)
                {
                    if (activeDoor)
                        activeDoor->SetState(DoorAnimState::Closing);

                    player.SetState(AnimState::Idle);
                    travelState = DoorTravelState::None;
                    activeDoor = nullptr;
                }
            }

            // Still update animations during teleport
            player.Update();
            kingPig.Update();
            for (Enemy& pig : minionPigs)
                pig.Update();
            kingPigDialogue.Update();
        }

        // RENDER 

        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);

        levelDesigner.Render(renderer);

        if (levelDesigner.GetActiveLevel() == 0)
        {
            doorLevel0To1.Render(renderer);
        }
        else if (levelDesigner.GetActiveLevel() == 1)
        {
            doorLevel1To0.Render(renderer);

            // Minion pigs
            for (Enemy& pig : minionPigs)
                pig.Render(renderer);

            // King Pig
            kingPig.Render(renderer);

            // Dialogue bubbles over King
            if (kingPigDialogue.IsPlaying())
                kingPigDialogue.Render(renderer, kingPig.GetX(), kingPig.GetY());
        }

        // Draw player only in the active level
        if (levelDesigner.GetActiveLevel() == playerLevelIndex)
            player.Render(renderer);

        // UI:life bar 
        int hearts = player.GetHealth();
        hearts = std::max(0, std::min(hearts, player.GetMaxHealth()));

        if (hearts > 0)
        {
            SDL_Rect src;
            src.x = 0;
            src.y = 0;
            src.w = HEART_W * hearts;
            src.h = LIVE_BAR_H;

            SDL_Rect dst;
            dst.x = 16;
            dst.y = 16;
            dst.w = src.w * 2;
            dst.h = src.h * 2;

            SDL_RenderCopy(renderer, liveBarTex, &src, &dst);
        }

        SDL_RenderPresent(renderer);
    }
    return 0;
}
