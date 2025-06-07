#ifdef __MINGW32__
#include "moon4k/substates/ResultsSubState.h"
#include "engine/core/Engine.h"
#include "engine/input/Input.h"
#include "engine/core/SDLManager.h"
#include "states/FreeplayState.h"
#elif defined(__SWITCH__)
#include "moon4k/substates/ResultsSubState.h"
#include "engine/core/Engine.h"
#include "engine/input/Input.h"
#include "engine/core/SDLManager.h"
#include "states/FreeplayState.h"
#else
#include "ResultsSubState.h"
#include "../../engine/core/Engine.h"
#include "../../engine/input/Input.h"
#include "../../engine/core/SDLManager.h"
#include "../states/FreeplayState.h"
#endif

ResultsSubState::ResultsSubState() :
    background(nullptr),
    graphBackground(nullptr),
    titleText(nullptr),
    scoreText(nullptr),
    accuracyText(nullptr),
    missesText(nullptr),
    totalHitText(nullptr),
    continueText(nullptr),
    rankText(nullptr),
    score(0),
    misses(0),
    accuracy(0.0f),
    totalNotesHit(0),
    rank("?"),
    backgroundAlpha(0.0f),
    textStartY(-55.0f),
    textTargetY(20.0f),
    animationTimer(0.0f)
{
    create();
}

ResultsSubState::~ResultsSubState() {
    destroy();
}

void ResultsSubState::create() {
    createVisuals();
}

void ResultsSubState::createVisuals() {
    Engine* engine = Engine::getInstance();
    
    background = new Sprite();
    background->setPosition(0, 0);
    SDL_Texture* bgTexture = SDL_CreateTexture(
        SDLManager::getInstance().getRenderer(),
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        engine->getWindowWidth(),
        engine->getWindowHeight()
    );
    SDL_SetTextureBlendMode(bgTexture, SDL_BLENDMODE_BLEND);
    background->setTexture(bgTexture);
    background->setAlpha(0.0f);

    titleText = new Text();
    titleText->setPosition(20, textStartY);
    titleText->setText("Song Complete!");
    titleText->setFormat("assets/fonts/vcr.ttf", 34, 0xFFFFFFFF);

    scoreText = new Text();
    scoreText->setPosition(20, textStartY + 20);
    scoreText->setText("Score: " + std::to_string(score));
    scoreText->setFormat("assets/fonts/vcr.ttf", 28, 0xFFFFFFFF);

    accuracyText = new Text();
    accuracyText->setPosition(20, textStartY + 40);
    accuracyText->setText("Accuracy: " + std::to_string(accuracy) + "%");
    accuracyText->setFormat("assets/fonts/vcr.ttf", 28, 0xFFFFFFFF);

    missesText = new Text();
    missesText->setPosition(20, textStartY + 60);
    missesText->setText("Misses: " + std::to_string(misses));
    missesText->setFormat("assets/fonts/vcr.ttf", 28, 0xFFFFFFFF);

    totalHitText = new Text();
    totalHitText->setPosition(20, textStartY + 80);
    totalHitText->setText("Total Notes Hit: " + std::to_string(totalNotesHit));
    totalHitText->setFormat("assets/fonts/vcr.ttf", 28, 0xFFFFFFFF);

    rankText = new Text();
    rankText->setPosition(20, textStartY + 100);
    rankText->setText("Rank: " + rank);
    rankText->setFormat("assets/fonts/vcr.ttf", 28, 0xFFFFFFFF);

    float contX = engine->getWindowWidth() - 475;
    float contY = engine->getWindowHeight() + 50;
    continueText = new Text();
    continueText->setPosition(contX, contY);
    continueText->setText("Press ENTER to continue.");
    continueText->setFormat("assets/fonts/vcr.ttf", 28, 0xFFFFFFFF);

    graphBackground = new Sprite();
    graphBackground->setPosition(engine->getWindowWidth() - 500, 45);
    SDL_Texture* graphTexture = SDL_CreateTexture(
        SDLManager::getInstance().getRenderer(),
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        450, 240
    );
    SDL_SetTextureBlendMode(graphTexture, SDL_BLENDMODE_BLEND);
    graphBackground->setTexture(graphTexture);
    graphBackground->setAlpha(0.0f);
}

void ResultsSubState::updateAnimations(float deltaTime) {
    animationTimer += deltaTime;
    
    if (backgroundAlpha < 0.5f) {
        backgroundAlpha += deltaTime * 2.0f;
        background->setAlpha(backgroundAlpha);
    }

    float progress = std::clamp(animationTimer * 2.0f, 0.0f, 1.0f);
    float easedProgress = 1.0f - std::pow(1.0f - progress, 4);

    titleText->setY(textStartY + (textTargetY - textStartY) * easedProgress);
    scoreText->setY(titleText->getY() + 40);
    accuracyText->setY(scoreText->getY() + 30);
    missesText->setY(accuracyText->getY() + 30);
    totalHitText->setY(missesText->getY() + 30);
    rankText->setY(totalHitText->getY() + 30);

    float contTargetY = Engine::getInstance()->getWindowHeight() - 45;
    continueText->setY(Engine::getInstance()->getWindowHeight() + 50 - (100 * easedProgress));
}

void ResultsSubState::update(float deltaTime) {
    Input::UpdateKeyStates();
    Input::UpdateControllerStates();
    
    updateAnimations(deltaTime);

    if (Input::justPressed(SDL_SCANCODE_RETURN) || Input::isControllerButtonJustPressed(SDL_CONTROLLER_BUTTON_START)) {
        Engine::getInstance()->switchState(new FreeplayState());
        getParentState()->closeSubState();
    }
}

void ResultsSubState::render() {
    SDL_SetRenderDrawBlendMode(SDLManager::getInstance().getRenderer(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(SDLManager::getInstance().getRenderer(), 0, 0, 0, static_cast<Uint8>(backgroundAlpha * 255));
    
    Engine* engine = Engine::getInstance();
    SDL_Rect overlay = {0, 0, engine->getWindowWidth(), engine->getWindowHeight()};
    SDL_RenderFillRect(SDLManager::getInstance().getRenderer(), &overlay);
    
    if (graphBackground) graphBackground->render();
    if (titleText) titleText->render();
    if (scoreText) scoreText->render();
    if (accuracyText) accuracyText->render();
    if (missesText) missesText->render();
    if (totalHitText) totalHitText->render();
    if (rankText) rankText->render();
    if (continueText) continueText->render();
}

void ResultsSubState::destroy() {
    cleanup();
}

void ResultsSubState::cleanup() {
    delete background;
    delete graphBackground;
    delete titleText;
    delete scoreText;
    delete accuracyText;
    delete missesText;
    delete totalHitText;
    delete rankText;
    delete continueText;

    background = nullptr;
    graphBackground = nullptr;
    titleText = nullptr;
    scoreText = nullptr;
    accuracyText = nullptr;
    missesText = nullptr;
    totalHitText = nullptr;
    rankText = nullptr;
    continueText = nullptr;
}

void ResultsSubState::setStats(int score, int misses, float accuracy, int totalNotesHit, const std::string& rank) {
    this->score = score;
    this->misses = misses;
    this->accuracy = accuracy;
    this->totalNotesHit = totalNotesHit;
    this->rank = rank;
    updateStatsText();
}

void ResultsSubState::updateStatsText() {
    if (scoreText) {
        scoreText->setText("Score: " + std::to_string(score));
    }
    if (accuracyText) {
        char accuracyStr[32];
        snprintf(accuracyStr, sizeof(accuracyStr), "Accuracy: %.2f%%", accuracy);
        accuracyText->setText(accuracyStr);
    }
    if (missesText) {
        missesText->setText("Misses: " + std::to_string(misses));
    }
    if (totalHitText) {
        totalHitText->setText("Total Notes Hit: " + std::to_string(totalNotesHit));
    }
    if (rankText) {
        rankText->setText("Rank: " + rank);
    }
}
