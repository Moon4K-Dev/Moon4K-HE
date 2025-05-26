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
nlohmann::json Note::cachedConfig;
std::string Note::currentNoteskin;

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
        cachedConfig = nlohmann::json();
        currentNoteskin.clear();
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
    std::string spritePath = "assets/images/ui-skins/" + noteskin + "/notes";

    if (!assetsLoaded || currentNoteskin != skin) {
        cachedConfig = loadConfig(configPath);
        
        if (cachedConfig.contains("offsets")) {
            offsets = cachedConfig["offsets"].get<std::vector<float>>();
        } else {
            offsets = {0.0f, 0.0f};
        }
        
        if (!assetsLoaded || currentNoteskin != skin) {
            sharedInstance->loadFrames(spritePath + ".png", spritePath + ".xml");
            currentNoteskin = skin;
            
            const char* noteTypes[] = {"left", "down", "up", "right"};
            const char* noteAnims[] = {"static", "press", "confirm", "note", "hold", "holdend"};
            int fps = cachedConfig.value("framerate", 24);

            for (const char* type : noteTypes) {
                std::string prefix = type;
                std::vector<std::string> frames;

                frames = {prefix + " static0000"};
                sharedInstance->addAnimation(type + std::string("_static"), frames, fps, false);

                frames = {prefix + " press0000", prefix + " press0001", prefix + " press0002"};
                sharedInstance->addAnimation(type + std::string("_press"), frames, fps, false);

                frames = {prefix + " confirm0000", prefix + " confirm0001", prefix + " confirm0002"};
                sharedInstance->addAnimation(type + std::string("_confirm"), frames, fps, false);

                frames = {prefix + " note0000"};
                sharedInstance->addAnimation(type + std::string("_note"), frames, fps, false);

                frames = {prefix + " hold0000"};
                sharedInstance->addAnimation(type + std::string("_hold"), frames, fps, false);

                frames = {prefix + " hold end0000"};
                sharedInstance->addAnimation(type + std::string("_holdend"), frames, fps, false);
            }
            
            noteTexture = sharedInstance->shareTexture();
            assetsLoaded = true;
        }
    }

    setTexture(noteTexture);
    copyFramesFrom(*sharedInstance);
    copyAnimationsFrom(*sharedInstance);

    const char* noteTypes[] = {"left", "down", "up", "right"};
    std::string prefix = noteTypes[direction];

    float size = cachedConfig.value("size", 0.7f);
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
    const char* noteTypes[] = {"left", "down", "up", "right"};
    std::string prefix = noteTypes[direction];
    playAnimation(prefix + "_" + anim);
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
                tooLate = false;
            } else {
                canBeHit = false;
            }
        } else {
            if (strumTime > Conductor::songPosition - Conductor::safeZoneOffset * 0.3f &&
                strumTime < Conductor::songPosition + Conductor::safeZoneOffset * 0.2f) {
                canBeHit = true;
                tooLate = false;
            } else {
                canBeHit = false;
            }
        }
        
        if (strumTime < Conductor::songPosition - Conductor::safeZoneOffset * 2.0f && !wasGoodHit) {
            tooLate = true;
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

        if (strumTime < Conductor::songPosition - Conductor::safeZoneOffset && !wasGoodHit) {
            tooLate = true;
        }
    }
}

Note::~Note() {
    texture = nullptr;
    
    frames.clear();
    noteAnimations.clear();
}