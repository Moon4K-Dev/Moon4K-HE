#include "TitleState.h"
#include "../../engine/core/Engine.h"
#include "../../engine/input/Input.h"
#include "../../engine/audio/SoundManager.h"
#include <random>
#include <algorithm>

TitleState* TitleState::instance = nullptr;

TitleState::TitleState() 
    : titleSprite(nullptr)
    , songText(nullptr) {
    instance = this;
    
    titleMusic = {
        "Lofi Music 1",
        "Lofi Music 2",
        "Trojan Virus v3 - VS Ron"
    };
}

TitleState::~TitleState() {
    destroy();
}

void TitleState::create() {
    Log::getInstance().info("TitleState::create() - Starting...");
    Engine* engine = Engine::getInstance();
    
    SDL_SetRenderDrawColor(SDLManager::getInstance().getRenderer(), 0, 0, 0, 255);

    titleSprite = new Sprite(Paths::image("sexylogobyhiro"));    
    titleSprite->setScale(0.45f, 0.45f);
    titleSprite->setPosition(
        (engine->getWindowWidth() - titleSprite->getWidth() * 0.45f) / 2,
        -50 + (engine->getWindowHeight() - titleSprite->getHeight() * 0.45f) / 2
    );
    engine->addSprite(titleSprite);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, titleMusic.size() - 1);
    currentMusic = titleMusic[dis(gen)];
    
    std::string musicPath = "title/" + currentMusic;
    SoundManager::getInstance().playMusic(Paths::music(musicPath), true);
    SoundManager::getInstance().setMusicVolume(0.5f);

    songText = new Text();
    songText->setFormat(Paths::font("vcr.ttf"), 16, 0xFFFFFFFF);
    songText->setText("Now Playing: " + currentMusic);
    songText->setPosition(10, 10);
    engine->addText(songText);

    Log::getInstance().info("TitleState initialized successfully! Playing: " + currentMusic);
}

void TitleState::update(float deltaTime) {
    SwagState::update(deltaTime);
    Input::UpdateKeyStates();
    Input::UpdateControllerStates();

    if (Input::justPressed(SDL_SCANCODE_RETURN) || 
        Input::justPressed(SDL_SCANCODE_SPACE) ||
        Input::isControllerButtonJustPressed(SDL_CONTROLLER_BUTTON_START)) {        
        Engine::getInstance()->switchState(new FreeplayState());
    }
}

void TitleState::render() {
    if (titleSprite) {
        titleSprite->render();
    }
    if (songText) {
        songText->render();
    }
}

void TitleState::destroy() {
    titleSprite = nullptr;
    songText = nullptr;    
}