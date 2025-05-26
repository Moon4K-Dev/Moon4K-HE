#include "Note.h"
#include "../states/PlayState.h"
#include "../game/Conductor.h"
#include "../game/GameConfig.h"
#include "../../engine/utils/Log.h"
#include "../backend/json.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

const float Note::STRUM_X = 42.0f;
const float Note::swagWidth = 160.0f * 0.7f;
bool Note::assetsLoaded = false;
SDL_Texture* Note::noteTexture = nullptr;
AnimatedSprite* Note::sharedInstance = nullptr;
std::map<std::string, AnimatedSprite::Animation> Note::noteAnimations;

void Note::loadAssets() {
    if (!assetsLoaded) {
        sharedInstance = new AnimatedSprite();
        assetsLoaded = true;
    }
}

void Note::unloadAssets() {
    if (assetsLoaded) {
        if (noteTexture) {
            SDL_DestroyTexture(noteTexture);
            noteTexture = nullptr;
        }
        if (sharedInstance) {
            delete sharedInstance;
            sharedInstance = nullptr;
        }
        noteAnimations.clear();
        assetsLoaded = false;
    }
}

nlohmann::json Note::loadConfig(const std::string& configPath) {
    std::ifstream file(configPath);
    if (!file.is_open()) {
        Log::getInstance().error("Failed to load noteskin config: " + configPath);
        return nlohmann::json();
    }

    try {
        nlohmann::json config;
        file >> config;
        return config;
    } catch (const std::exception& ex) {
        Log::getInstance().error("Failed to parse noteskin config: " + std::string(ex.what()));
        return nlohmann::json();
    }
}

Note::Note(float x, float y, int direction, float strumTime, const std::string& noteskin, bool isSustainNote, int keyCount) 
    : AnimatedSprite(), direction(direction), strumTime(strumTime), noteskin(noteskin), 
      isSustainNote(isSustainNote), keyCount(keyCount) {
    
    setPosition(x, y);
    setOrigPos();
    loadNoteSkin(noteskin, direction);
}

void Note::setOrigPos() {
    origPos = {getX(), getY()};
}

void Note::loadNoteSkin(const std::string& skin, int dir) {
    if (dir != -1) {
        direction = dir;
    }

    noteskin = skin;
    std::string configPath = "assets/images/ui-skins/" + noteskin + "/config.json";
    auto config = loadConfig(configPath);

    if (config.contains("offsets")) {
        offsets = config["offsets"].get<std::vector<float>>();
    } else {
        offsets = {0.0f, 0.0f};
    }

    std::string spritePath = "assets/images/ui-skins/" + noteskin + "/notes";
    loadFrames(spritePath + ".png", spritePath + ".xml");

    const char* noteTypes[] = {"left", "down", "up", "right"};
    const char* noteAnims[] = {"static", "press", "confirm", "note", "hold", "holdend"};
    int fps = config.value("framerate", 24);

    std::string prefix = noteTypes[direction];
    std::vector<std::string> frames;

    frames = {prefix + " static0000"};
    addAnimation("static", frames, fps, false);

    frames = {prefix + " press0000", prefix + " press0001", prefix + " press0002"};
    addAnimation("press", frames, fps, false);

    frames = {prefix + " confirm0000", prefix + " confirm0001", prefix + " confirm0002"};
    addAnimation("confirm", frames, fps, false);

    frames = {prefix + " note0000"};
    addAnimation("note", frames, fps, false);

    frames = {prefix + " hold0000"};
    addAnimation("hold", frames, fps, false);

    frames = {prefix + " hold end0000"};
    addAnimation("holdend", frames, fps, false);

    float size = config.value("size", 0.7f);
    setScale(size, size);
    updateHitbox();

    playAnim("note");

    if (isSustainNote && lastNote != nullptr) {
        alpha = 0.6f;
        x += width / 2;

        playAnim("holdend");
        updateHitbox();

        x -= width / 2;

        if (lastNote->isSustainNote) {
            lastNote->playAnim("hold");
            lastNote->scale.y *= Conductor::stepCrochet / 100 * 1.5f * PlayState::SONG.speed;
            lastNote->updateHitbox();
        }
    }
}

void Note::playAnim(const std::string& anim) {
    playAnimation(anim);
    setPosition(getX() + offsets[0], getY() + offsets[1]);
}

void Note::update(float deltaTime) {
    AnimatedSprite::update(deltaTime);
    scaleX = scale.x;
    scaleY = scale.y;
    calculateCanBeHit();
}

void Note::calculateCanBeHit() {
    if (isSustainNote) {
        if (shouldHit) {
            if (strumTime > Conductor::songPosition - (Conductor::safeZoneOffset * 1.5f) &&
                strumTime < Conductor::songPosition + (Conductor::safeZoneOffset * 0.5f)) {
                canBeHit = true;
            } else {
                canBeHit = false;
            }
        } else {
            if (strumTime > Conductor::songPosition - Conductor::safeZoneOffset * 0.3f &&
                strumTime < Conductor::songPosition + Conductor::safeZoneOffset * 0.2f) {
                canBeHit = true;
            } else {
                canBeHit = false;
            }
        }
    } else {
        if (shouldHit) {
            if (strumTime > Conductor::songPosition - Conductor::safeZoneOffset &&
                strumTime < Conductor::songPosition + Conductor::safeZoneOffset) {
                canBeHit = true;
            } else {
                canBeHit = false;
            }
        } else {
            if (strumTime > Conductor::songPosition - Conductor::safeZoneOffset * 0.3f &&
                strumTime < Conductor::songPosition + Conductor::safeZoneOffset * 0.2f) {
                canBeHit = true;
            } else {
                canBeHit = false;
            }
        }
    }

    if (strumTime < Conductor::songPosition - Conductor::safeZoneOffset && !wasGoodHit) {
        tooLate = true;
    }
}

Note::~Note() {}