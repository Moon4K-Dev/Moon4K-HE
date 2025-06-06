#pragma once

#include "../../engine/graphics/Text.h"
#include "../../engine/graphics/Sprite.h"
#include "../../engine/graphics/Button.h"
#include "../../engine/core/State.h"
#include <memory>
#include <string>

class UI {
private:
    std::unique_ptr<Text> scoreTxt;
    std::unique_ptr<Text> notesHitTxt;
    std::unique_ptr<Text> accTxt;
    std::unique_ptr<Sprite> healthBarBG;
    std::unique_ptr<Sprite> healthBarFG;
    std::unique_ptr<Text> songTimerTxt;
    std::unique_ptr<Sprite> laneUnderlay;

    float health;
    int score;
    int notesHit;
    float accuracy;
    float songLength;
    float currentTime;

    std::string formatTime(float seconds);

public:
    UI();
    ~UI();

    void init();
    void update(float deltaTime);
    void render();

    void setHealth(float value);
    void setScore(int value);
    void setNotesHit(int value);
    void setAccuracy(float value);
    void setSongTime(float current, float total);

    float getHealth() const { return health; }
    int getScore() const { return score; }
    int getNotesHit() const { return notesHit; }
    float getAccuracy() const { return accuracy; }
};
