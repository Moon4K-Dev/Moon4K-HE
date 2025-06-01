#pragma once

#include "../SwagState.h"
#include "../../engine/graphics/Sprite.h"
#include "../../engine/graphics/Text.h"
#include "../../engine/utils/Log.h"
#include "../../engine/utils/Paths.h"
#include "../../engine/core/SDLManager.h"
#include "../../engine/core/Engine.h"
#include <vector>

class TitleState;

class SplashState : public SwagState {
public:
    static SplashState* instance;
    
    SplashState();
    ~SplashState();

    void create() override;
    void update(float deltaTime) override;
    void render() override;
    void destroy() override;

private:
    Sprite* arrowsexylogo;
    Text* funnyText;
    Text* wackyText;
    std::vector<std::string> introTexts;
    std::string curWacky;
    float transitionTimer;
    bool titleStarted;
    float logoTweenDelay;
    float textTweenDelay;
    float wackyTextDelay;
    float funnyTextStartY;
    float funnyTextTargetY;
    bool funnyTextReachedTarget;
};