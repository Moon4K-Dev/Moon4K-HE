#pragma once

#include "../engine/core/State.h"
#include "../engine/audio/Sound.h"
#include "../engine/core/Engine.h"
#include "game/Conductor.h"
#include <SDL2/SDL.h>
#ifdef __MINGW32__ 
#elif defined(__SWITCH__)
#else
#include "../engine/utils/Discord.h"
#endif

class SwagState : public State {
public:
    SwagState();
    virtual ~SwagState();

    void create() override;
    void update(float deltaTime) override;
    void render() override;
    void destroy() override;

    void updateBeat();
    void updateCurStep();
    void stepHit();
    void beatHit();

    void startTransitionIn(float duration = 0.5f);
    void startTransitionOut(float duration = 0.5f);
    bool isTransitioning() const { return transitionActive; }

    static const std::string soundExt;
    int curStep = 0;
    int curBeat = 0;
    float lastBeat = 0.0f;
    float lastStep = 0.0f;

protected:
    bool transitionActive = false;
    bool transitioningIn = false;
    float transitionAlpha = 0.0f;
    float transitionDuration = 0.5f;
    float transitionTimer = 0.0f;
    SDL_Texture* transitionTexture = nullptr;
    void updateTransition(float deltaTime);
    void renderTransition();
}; 