#pragma once

#include "../../engine/graphics/AnimatedSprite.h"
#include <map>
#include <string>

class Note : public AnimatedSprite {
public:
    // Note types in FNF order: Left, Down, Up, Right
    static constexpr int LEFT_NOTE = 0;
    static constexpr int DOWN_NOTE = 1;
    static constexpr int UP_NOTE = 2;
    static constexpr int RIGHT_NOTE = 3;

    static const float STRUM_X;
    static const float swagWidth;

    static bool assetsLoaded;
    static SDL_Texture* noteTexture;
    static AnimatedSprite* sharedInstance;
    static std::map<std::string, AnimatedSprite::Animation> noteAnimations;

    static void loadAssets();
    static void unloadAssets();
    static float getTargetY();

    Note(float strumTime, int noteData, Note* prevNote = nullptr, bool sustainNote = false);
    ~Note();

    void update(float deltaTime) override;
    void setupNote();
    void setupSustainNote();

    float strumTime;
    int noteData;
    float sustainLength;
    bool mustPress;
    bool isSustainNote;
    bool canBeHit;
    bool wasGoodHit;
    bool tooLate;
    float noteScore;
    Note* prevNote;
    bool kill = false;
}; 