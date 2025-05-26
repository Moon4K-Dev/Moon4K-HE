#pragma once

#include "../../engine/graphics/AnimatedSprite.h"
#include <map>
#include <string>
#include <vector>
#include "../backend/json.hpp"

class Note : public AnimatedSprite {
public:
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

    Note(float x, float y, int direction = 0, float strumTime = 0.0f, const std::string& noteskin = "default", bool isSustainNote = false, int keyCount = 4);
    ~Note();

    void update(float deltaTime) override;
    void setupNote();
    void setupSustainNote();
    void loadNoteSkin(const std::string& noteskin = "default", int direction = -1);
    void playAnim(const std::string& anim);
    void calculateCanBeHit();
    void setOrigPos();

    void setY(float y) { setPosition(getX(), y); }
    float getY() const { return y; }

    std::string noteskin = "default";
    int direction = 0;
    float strumTime = 0.0f;
    bool isSustainNote = false;
    bool shouldHit = true;
    float sustainLength = 0.0f;
    bool canBeHit = false;
    bool tooLate = false;
    bool wasGoodHit = false;
    int keyCount = 4;
    float scaleX = 0.0f;
    float scaleY = 0.0f;
    Note* lastNote = nullptr;
    std::vector<float> origPos = {0.0f, 0.0f};
    int rawNoteData = 0;
    std::vector<float> offsets = {0.0f, 0.0f};
    bool isEndNote = false;
    Note* nextNote = nullptr;
    bool kill = false;

private:
    nlohmann::json loadConfig(const std::string& configPath);
}; 