#include "SwagState.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include "game/Conductor.h"
#include "../engine/core/SDLManager.h"
#include "../engine/core/Engine.h"

const std::string SwagState::soundExt = ".ogg";

SwagState::SwagState()
    : lastBeat(0.0f)
    , lastStep(0.0f)
    , curStep(0)
    , curBeat(0)
    , transitionActive(false)
    , transitioningIn(false)
    , transitionAlpha(0.0f)
    , transitionDuration(0.5f)
    , transitionTimer(0.0f)
    , transitionTexture(nullptr) {
    
    SDL_Renderer* renderer = SDLManager::getInstance().getRenderer();
    int width = Engine::getInstance()->getWindowWidth();
    int height = Engine::getInstance()->getWindowHeight();
    
    transitionTexture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        width,
        height
    );
    
    if (transitionTexture) {
        SDL_SetTextureBlendMode(transitionTexture, SDL_BLENDMODE_BLEND);
    }
}

SwagState::~SwagState() {
    destroy();
}

void SwagState::create() {
    std::cout << "SwagState::create()" << std::endl;
    startTransitionIn();
}

void SwagState::update(float elapsed) {
    int oldStep = curStep;

    updateCurStep();
    updateBeat();

    if (oldStep != curStep && curStep > 0) {
        stepHit();
    }

    if (_subStates.empty()) {
        updateTransition(elapsed);
    } else {
        updateSubState(elapsed);
    }
}

void SwagState::render() {
    if (!_subStates.empty()) {
        renderSubState();
    }
    renderTransition();
}

void SwagState::destroy() {
    if (transitionTexture) {
        SDL_DestroyTexture(transitionTexture);
        transitionTexture = nullptr;
    }
}

void SwagState::startTransitionIn(float duration) {
    transitionActive = true;
    transitioningIn = true;
    transitionDuration = duration;
    transitionTimer = 0.0f;
    transitionAlpha = 1.0f;
}

void SwagState::startTransitionOut(float duration) {
    transitionActive = true;
    transitioningIn = false;
    transitionDuration = duration;
    transitionTimer = 0.0f;
    transitionAlpha = 0.0f;
}

void SwagState::updateTransition(float deltaTime) {
    if (!transitionActive) return;

    transitionTimer += deltaTime;
    float progress = std::clamp(transitionTimer / transitionDuration, 0.0f, 1.0f);

    if (transitioningIn) {
        transitionAlpha = 1.0f - progress;
    } else {
        transitionAlpha = progress;
    }

    if (progress >= 1.0f) {
        transitionActive = false;
    }
}

void SwagState::renderTransition() {
    if (!transitionActive || !transitionTexture) return;

    SDL_Renderer* renderer = SDLManager::getInstance().getRenderer();
    
    Uint8 alpha = static_cast<Uint8>(transitionAlpha * 255);
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, alpha);
    SDL_Rect fullscreen = {0, 0, Engine::getInstance()->getWindowWidth(), Engine::getInstance()->getWindowHeight()};
    SDL_RenderFillRect(renderer, &fullscreen);
}

void SwagState::updateBeat() {
    curBeat = static_cast<int>(std::floor(curStep / 4.0f));
}

void SwagState::updateCurStep() {
    BPMChangeEvent lastChange = {
        0,      // stepTime
        0.0f,   // songTime
        0       // bpm
    };

    for (const auto& change : Conductor::bpmChangeMap) {
        if (Conductor::songPosition >= change.songTime) {
            lastChange = change;
        }
    }

    float stepsSinceChange = (Conductor::songPosition - lastChange.songTime) / Conductor::stepCrochet;
    curStep = lastChange.stepTime + static_cast<int>(std::floor(stepsSinceChange));
}

void SwagState::stepHit() {
    if (curStep % 4 == 0) {
        beatHit();
    }
}

void SwagState::beatHit() {
    // become GAY!
}
