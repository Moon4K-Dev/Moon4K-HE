#include "Note.h"
#include "../states/PlayState.h"
#include "../game/Conductor.h"
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
        
        SDL_Surface* surface = IMG_Load("assets/images/NOTE_assets.png");
        if (!surface) {
            std::cerr << "Failed to load note texture: " << IMG_GetError() << std::endl;
            return;
        }

        noteTexture = SDL_CreateTextureFromSurface(SDLManager::getInstance().getRenderer(), surface);
        SDL_FreeSurface(surface);

        if (!noteTexture) {
            std::cerr << "Failed to create note texture: " << SDL_GetError() << std::endl;
            return;
        }

        sharedInstance->loadFrames("assets/images/NOTE_assets.png", "assets/images/NOTE_assets.xml");

        const char* noteTypes[] = {"purple", "blue", "green", "red"};
        for (int i = 0; i < 4; i++) {
            std::vector<std::string> scrollFrames = {std::string(noteTypes[i]) + "0000"};
            sharedInstance->addAnimation("scroll_" + std::string(noteTypes[i]), scrollFrames, 24, false);

            std::vector<std::string> holdEndFrames = {std::string(noteTypes[i]) + " hold end"};
            sharedInstance->addAnimation("end_" + std::string(noteTypes[i]), holdEndFrames, 24, false);

            std::vector<std::string> holdFrames = {std::string(noteTypes[i]) + " hold piece"};
            sharedInstance->addAnimation("hold_" + std::string(noteTypes[i]), holdFrames, 24, false);
        }

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

Note::Note(float strumTime, int noteData, Note* prevNote, bool sustainNote) 
    : AnimatedSprite(), strumTime(strumTime), noteData(noteData), prevNote(prevNote), 
      isSustainNote(sustainNote), sustainLength(0), mustPress(false), canBeHit(false),
      tooLate(false), wasGoodHit(false), noteScore(1.0f) {
    
    if (!assetsLoaded) {
        loadAssets();
    }
    setTexture(noteTexture);
    
    std::string noteType;
    if (noteData == LEFT_NOTE) {
        noteType = "purple";
    }
    else if (noteData == DOWN_NOTE) {
        noteType = "blue";
    }
    else if (noteData == UP_NOTE) {
        noteType = "green";
    }
    else if (noteData == RIGHT_NOTE) {
        noteType = "red";
    }
    else {
        noteType = "purple";
        Log::getInstance().info("Unknown note type: " + std::to_string(noteData));
    }

    loadFrames("assets/images/NOTE_assets.png", "assets/images/NOTE_assets.xml");

    std::string scrollAnim = "scroll_" + noteType;
    std::string holdAnim = "hold_" + noteType;
    std::string endAnim = "end_" + noteType;

    std::vector<std::string> scrollFrames = {noteType + "0000"};
    addAnimation(scrollAnim, scrollFrames, 24, false);

    std::vector<std::string> holdFrames = {noteType + " hold piece0000"};
    addAnimation(holdAnim, holdFrames, 24, false);

    std::vector<std::string> endFrames = {noteType + " hold end0000"};
    addAnimation(endAnim, endFrames, 24, false);

    if (sustainNote) {
        playAnimation(holdAnim);
    } else {
        playAnimation(scrollAnim);
    }

    setScale(0.7f, 0.7f);
    setVisible(true);
}

void Note::setupNote() {
    std::string colorPrefix;
    switch (noteData) {
        case LEFT_NOTE:
            colorPrefix = "purple";
            break;
        case DOWN_NOTE:
            colorPrefix = "blue";
            break;
        case UP_NOTE:
            colorPrefix = "green";
            break;
        case RIGHT_NOTE:
            colorPrefix = "red";
            break;
    }

    playAnimation("scroll_" + colorPrefix);
    x += swagWidth * noteData;
}

void Note::setupSustainNote() {
    noteScore *= 0.2f;
    alpha = 0.6f;

    x += width / 2;

    std::string colorPrefix;
    switch (noteData) {
        case LEFT_NOTE:
            colorPrefix = "purple";
            break;
        case DOWN_NOTE:
            colorPrefix = "blue";
            break;
        case UP_NOTE:
            colorPrefix = "green";
            break;
        case RIGHT_NOTE:
            colorPrefix = "red";
            break;
    }

    playAnimation("end_" + colorPrefix);
    updateHitbox();
    x -= width / 2;

    if (prevNote && prevNote->isSustainNote) {
        prevNote->playAnimation("hold_" + colorPrefix);
        prevNote->scale.y *= Conductor::stepCrochet / 100 * 1.5 * PlayState::SONG.speed;
        prevNote->updateHitbox();
    }
}

float Note::getTargetY() {
    int windowHeight = Engine::getInstance()->getWindowHeight();
    if (GameConfig::getInstance()->isDownscroll()) {
        return windowHeight - 150.0f;
    }
    return 50.0f;
}

void Note::update(float deltaTime) {
    AnimatedSprite::update(deltaTime);

    float songPos = Conductor::songPosition;
    float scrollSpeed = PlayState::SONG.speed;
    
    float targetY = getTargetY();
    
    float timeDiff = strumTime - songPos;
    float distance = timeDiff * 0.45f * scrollSpeed;
    
    float x = getX();
    float y = targetY + (GameConfig::getInstance()->isDownscroll() ? -distance : distance);
    
    setPosition(x, y);
    setVisible(true);

    if (mustPress) {
        if (strumTime > songPos - Conductor::safeZoneOffset &&
            strumTime < songPos + (Conductor::safeZoneOffset * 0.5f)) {
            canBeHit = true;
        } else {
            canBeHit = false;
        }

        if (strumTime < songPos - Conductor::safeZoneOffset && !wasGoodHit) {
            tooLate = true;
        }
    } else {
        canBeHit = false;
        if (strumTime <= songPos) {
            wasGoodHit = true;
        }
    }
}

Note::~Note() {}