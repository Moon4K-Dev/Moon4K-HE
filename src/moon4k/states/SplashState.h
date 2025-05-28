#pragma once

#include "../../engine/core/State.h"
#include "../../engine/graphics/Sprite.h"
#include "../../engine/graphics/Text.h"
#include "../SwagState.h"
#include "../../engine/utils/Log.h"
#include "../../engine/utils/Paths.h"
//#include "TitleState.h"

class SplashState : public SwagState {
public:
    static SplashState* instance;
    
    SplashState();
    ~SplashState() override;
    
    void create() override;
    void update(float deltaTime) override;
    void render() override;
    void destroy() override;

private:
    Sprite* arrowsexylogo;
    Text* funnyText;
    Text* wackyText;
    float transitionTimer;
    bool titleStarted;
    std::vector<std::string> introTexts;
    std::string curWacky;
    float logoTweenDelay;
    float textTweenDelay;
    float wackyTextDelay;
    float funnyTextStartY;
    float funnyTextTargetY;
    bool funnyTextReachedTarget;
};