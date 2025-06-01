#include "SplashState.h"
#include "TitleState.h"
#include "../../engine/core/Engine.h"
#include "../../engine/input/Input.h"
#include "../../engine/audio/SoundManager.h"
#include <random>
#include <algorithm>

SplashState* SplashState::instance = nullptr;

SplashState::SplashState() 
    : arrowsexylogo(nullptr)
    , funnyText(nullptr)
    , wackyText(nullptr)
    , transitionTimer(0.0f)
    , titleStarted(false)
    , logoTweenDelay(0.5f)
    , textTweenDelay(0.5f)
    , wackyTextDelay(1.5f)
    , funnyTextStartY(0.0f)
    , funnyTextTargetY(0.0f)
    , funnyTextReachedTarget(false) {
    instance = this;
    
    introTexts = {
        "Look Ma, I'm in a video game!",
        "Swag Swag Cool Shit",
        "I love ninjamuffin99",
        "Follow @maybekoi_ on twitter!",
        "Inspired by FNF and OSU!Mania",
        "The first o in the Moon 4k logo stole government files, das why it's hidden lol!"
    };
}

SplashState::~SplashState() {
    destroy();
}

void SplashState::create() {
    SwagState::create();

    Engine* engine = Engine::getInstance();
    
    SDL_SetRenderDrawColor(SDLManager::getInstance().getRenderer(), 0, 0, 0, 255);
    
    arrowsexylogo = new Sprite(Paths::image("splash/notelogo"));
    arrowsexylogo->setScale(0.3f, 0.3f);
    arrowsexylogo->setPosition(
        (engine->getWindowWidth() - arrowsexylogo->getWidth() * 0.3f) / 2,
        (engine->getWindowHeight() - arrowsexylogo->getHeight() * 0.3f) / 2 - 35
    );
    arrowsexylogo->setAlpha(0.0f);
    engine->addSprite(arrowsexylogo);

    funnyTextStartY = arrowsexylogo->getY() + arrowsexylogo->getHeight() * 0.3f + 35;
    funnyTextTargetY = funnyTextStartY + 35;

    funnyText = new Text();
    funnyText->setFormat(Paths::font("vcr.ttf"), 24, 0xFFFFFFFF);
    funnyText->setText("Welcome to Moon4K!");
    funnyText->setPosition(
        (engine->getWindowWidth() - funnyText->getWidth()) / 2,
        funnyTextStartY
    );
    funnyText->setAlpha(0.0f);
    engine->addText(funnyText);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, introTexts.size() - 1);
    curWacky = introTexts[dis(gen)];
    
    wackyText = new Text();
    wackyText->setFormat(Paths::font("vcr.ttf"), 24, 0xFFFFFFFF);
    wackyText->setText(curWacky);
    wackyText->setPosition(
        (engine->getWindowWidth() - wackyText->getWidth()) / 2,
        funnyTextTargetY
    );
    wackyText->setAlpha(0.0f);
    engine->addText(wackyText);

    Discord::GetInstance().SetDetails("Just booted up Moon4K!");
    Discord::GetInstance().Update();

    Log::getInstance().info("SplashState initialized!");
}

void SplashState::update(float deltaTime) {
    SwagState::update(deltaTime);
    Input::UpdateKeyStates();
    Input::UpdateControllerStates();

    if (!isTransitioning()) {
        if (logoTweenDelay > 0) {
            logoTweenDelay -= deltaTime;
        } else if (arrowsexylogo->getAlpha() < 1.0f) {
            float newAlpha = arrowsexylogo->getAlpha() + deltaTime;
            arrowsexylogo->setAlpha(std::min<float>(1.0f, newAlpha));
            arrowsexylogo->setY(arrowsexylogo->getY() - deltaTime * 35.0f);
        }

        if (textTweenDelay > 0) {
            textTweenDelay -= deltaTime;
        } else if (funnyText->getAlpha() < 1.0f && wackyTextDelay > 0) {
            float newAlpha = funnyText->getAlpha() + deltaTime;
            funnyText->setAlpha(std::min<float>(1.0f, newAlpha));
            
            if (!funnyTextReachedTarget) {
                float newY = funnyText->getY() + deltaTime * 35.0f;
                if (newY >= funnyTextTargetY) {
                    newY = funnyTextTargetY;
                    funnyTextReachedTarget = true;
                }
                funnyText->setY(newY);
            }
        }

        if (wackyTextDelay > 0) {
            wackyTextDelay -= deltaTime;
        } else {
            if (funnyText->getAlpha() > 0.0f) {
                float newAlpha = funnyText->getAlpha() - deltaTime;
                funnyText->setAlpha(std::max<float>(0.0f, newAlpha));
            }
            
            if (funnyText->getAlpha() < 0.1f) {
                float newWackyAlpha = wackyText->getAlpha() + deltaTime;
                wackyText->setAlpha(std::min<float>(1.0f, newWackyAlpha));
            }
        }

        if (wackyText->getAlpha() >= 1.0f) {
            transitionTimer += deltaTime;
            if (transitionTimer >= 5.5f && !titleStarted) {
                titleStarted = true;
                Log::getInstance().info("tryna switch to title state");
                startTransitionOut(0.5f);
                Engine::getInstance()->switchState(new TitleState());
            }
        }
    }
}

void SplashState::render() {
    arrowsexylogo->render();
    funnyText->render();
    wackyText->render();
    
    SwagState::render();
}

void SplashState::destroy() {
    arrowsexylogo = nullptr;
    funnyText = nullptr;
    wackyText = nullptr;
}