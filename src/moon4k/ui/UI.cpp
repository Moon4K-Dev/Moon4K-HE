#include "UI.h"
#include "../../engine/core/Engine.h"
#include "../../engine/core/SDLManager.h"
#include "../game/GameConfig.h"
#include "../../engine/utils/Paths.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

UI::UI() : 
    health(1.0f),
    score(0),
    notesHit(0),
    accuracy(0.0f),
    songLength(0.0f),
    currentTime(0.0f)
{
    init();
}

UI::~UI() = default;

void UI::init() {
    scoreTxt = std::make_unique<Text>();
    scoreTxt->setFormat(Paths::font("Zero G.ttf"), 26, 0xFFFFFFFF);
    scoreTxt->setText("Score: 0");

    notesHitTxt = std::make_unique<Text>();
    notesHitTxt->setFormat(Paths::font("Zero G.ttf"), 26, 0xFFFFFFFF);
    notesHitTxt->setText("0x");

    accTxt = std::make_unique<Text>();
    accTxt->setFormat(Paths::font("Zero G.ttf"), 26, 0xFFFFFFFF);
    accTxt->setText("Accuracy: 0%");

    songTimerTxt = std::make_unique<Text>();
    songTimerTxt->setFormat(Paths::font("Zero G.ttf"), 26, 0xFFFFFFFF);
    songTimerTxt->setText("0:00 / 0:00");

    healthBarBG = std::make_unique<Sprite>();
    healthBarFG = std::make_unique<Sprite>();
    laneUnderlay = std::make_unique<Sprite>();

    SDL_Renderer* renderer = SDLManager::getInstance().getRenderer();
    
    SDL_Surface* bgSurface = SDL_CreateRGBSurface(0, 150, 10, 32, 0, 0, 0, 0);
    SDL_FillRect(bgSurface, nullptr, SDL_MapRGB(bgSurface->format, 0, 0, 0));
    SDL_Texture* bgTexture = SDL_CreateTextureFromSurface(renderer, bgSurface);
    SDL_FreeSurface(bgSurface);
    healthBarBG->setTexture(bgTexture);

    SDL_Surface* fgSurface = SDL_CreateRGBSurface(0, 146, 6, 32, 0, 0, 0, 0);
    SDL_FillRect(fgSurface, nullptr, SDL_MapRGB(fgSurface->format, 255, 255, 255));
    SDL_Texture* fgTexture = SDL_CreateTextureFromSurface(renderer, fgSurface);
    SDL_FreeSurface(fgSurface);
    healthBarFG->setTexture(fgTexture);

    SDL_Surface* underlaySurface = SDL_CreateRGBSurface(0, 400, 720, 32, 0, 0, 0, 0);
    SDL_FillRect(underlaySurface, nullptr, SDL_MapRGBA(underlaySurface->format, 0, 0, 0, 164));
    SDL_Texture* underlayTexture = SDL_CreateTextureFromSurface(renderer, underlaySurface);
    SDL_SetTextureBlendMode(underlayTexture, SDL_BLENDMODE_BLEND);
    SDL_FreeSurface(underlaySurface);
    laneUnderlay->setTexture(underlayTexture);

    float screenWidth = static_cast<float>(Engine::getInstance()->getWindowWidth());
    float screenHeight = static_cast<float>(Engine::getInstance()->getWindowHeight());    
    bool downscroll = GameConfig::getInstance()->getDownscroll();
    float textStartY = downscroll ? screenHeight - 120 : 30;

    healthBarBG->setPosition(screenWidth - 160, downscroll ? screenHeight - 30 : 10);
    healthBarFG->setPosition(healthBarBG->getX() + 2, healthBarBG->getY() + 2);

    scoreTxt->setPosition(screenWidth - 310, textStartY);
    notesHitTxt->setPosition(screenWidth - 310, textStartY + 30);
    accTxt->setPosition(screenWidth - 310, textStartY + 60);

    float timerY = downscroll ? 50 : screenHeight - 30;
    songTimerTxt->setPosition(screenWidth - 310, timerY);

    laneUnderlay->setPosition(screenWidth / 2 - 200, 0);
    laneUnderlay->setAlpha(0.65f);
}

void UI::update(float deltaTime) {
    scoreTxt->setText("Score: " + std::to_string(score));
    notesHitTxt->setText(std::to_string(notesHit) + "x");
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << (accuracy * 100);
    accTxt->setText("Accuracy: " + ss.str() + "%");
    
    songTimerTxt->setText(formatTime(currentTime) + " / " + formatTime(songLength));
    healthBarFG->setScale(health, 1.0f);
}

void UI::render() {
    laneUnderlay->render();
    
    healthBarBG->render();
    healthBarFG->render();
    
    scoreTxt->render();
    notesHitTxt->render();
    accTxt->render();
    songTimerTxt->render();

}

std::string UI::formatTime(float seconds) {
    int minutes = static_cast<int>(seconds) / 60;
    int remainingSeconds = static_cast<int>(seconds) % 60;
    
    std::stringstream ss;
    ss << minutes << ":" << std::setfill('0') << std::setw(2) << remainingSeconds;
    return ss.str();
}

void UI::setHealth(float value) {
    health = std::clamp(value, 0.0f, 1.0f);
}

void UI::setScore(int value) {
    score = value;
}

void UI::setNotesHit(int value) {
    notesHit = value;
}

void UI::setAccuracy(float value) {
    accuracy = std::clamp(value, 0.0f, 1.0f);
}

void UI::setSongTime(float current, float total) {
    currentTime = current;
    songLength = total;
}
