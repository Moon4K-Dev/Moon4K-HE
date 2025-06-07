#pragma once

#ifdef __MINGW32__
#include "engine/core/SubState.h"
#include "engine/graphics/Sprite.h"
#include "engine/graphics/Text.h"
#elif defined(__SWITCH__)
#include "engine/core/SubState.h"
#include "engine/graphics/Sprite.h"
#include "engine/graphics/Text.h"
#else
#include "../../engine/core/SubState.h"
#include "../../engine/graphics/Sprite.h"
#include "../../engine/graphics/Text.h"
#endif

class ResultsSubState : public SubState {
public:
    ResultsSubState();
    ~ResultsSubState();

    void create() override;
    void update(float deltaTime) override;
    void render() override;
    void destroy() override;

    void setStats(int score, int misses, float accuracy, int totalNotesHit, const std::string& rank);

    int score;
    int misses;
    float accuracy;
    int totalNotesHit;
    std::string rank;

private:
    Sprite* background;
    Sprite* graphBackground;
    
    Text* titleText;
    Text* scoreText;
    Text* accuracyText;
    Text* missesText;
    Text* totalHitText;
    Text* continueText;
    Text* rankText;

    float backgroundAlpha;
    float textStartY;
    float textTargetY;
    float animationTimer;

    void createVisuals();
    void updateAnimations(float deltaTime);
    void updateStatsText();
    void cleanup();
};
