#include "MainMenuState.h"
#include "../../engine/core/Engine.h"
#include "../../engine/input/Input.h"
#include "../../engine/core/SDLManager.h"
#include "../../engine/utils/Paths.h"
#include "FreeplayState.h"
#include "OnlineDLState.h"
#include "OptionsState.h"
#include "LoginState.h"
#include "../network/UserSession.h"

MainMenuState::MainMenuState() 
    : titleSprite(nullptr)
    , usernameText(nullptr)
    , currentSelection(0) {
    menuTexts = {"Solo", "Browse Online Levels", "Settings", "Exit"};
}

MainMenuState::~MainMenuState() {
    destroy();
}

void MainMenuState::create() {
    SwagState::create();

    Engine* engine = Engine::getInstance();
    
    SDL_SetRenderDrawColor(SDLManager::getInstance().getRenderer(), 0, 0, 0, 255);

    titleSprite = new Sprite(Paths::image("sexylogobyhiro"));    
    titleSprite->setScale(0.25f, 0.25f);
    titleSprite->setPosition(
        (engine->getWindowWidth() - titleSprite->getWidth() * 0.25f) / 2,
        50
    );
    engine->addSprite(titleSprite);

    if (UserSession::IsLoggedIn()) {
        usernameText = new Text();
        usernameText->setFormat(Paths::font("vcr.ttf"), 20, 0xFFFFFFFF);
        usernameText->setText("Logged in as: " + UserSession::GetUsername());
        usernameText->setPosition(
            10,
            10
        );
        engine->addText(usernameText);
    }

    float startY = engine->getWindowHeight() * 0.5f;
    for (size_t i = 0; i < menuTexts.size(); i++) {
        Text* txt = new Text();
        txt->setFormat(Paths::font("vcr.ttf"), 32, 0xFFFFFFFF);
        txt->setText(menuTexts[i]);
        txt->setPosition(
            (engine->getWindowWidth() - txt->getWidth()) / 2,
            startY + (i * 60)
        );
        menuItems.push_back(txt);
        engine->addText(txt);
    }

    changeSelection();

    char buffer[256];
    snprintf(buffer, sizeof(buffer), "In the Main Menu!");
    Discord::GetInstance().SetDetails(buffer);
    Discord::GetInstance().Update();
}

void MainMenuState::update(float deltaTime) {
    SwagState::update(deltaTime);

    if (!isTransitioning()) {
        Input::UpdateKeyStates();
        Input::UpdateControllerStates();

        if (Input::justPressed(SDL_SCANCODE_UP)) {
            changeSelection(-1);
        }
        if (Input::justPressed(SDL_SCANCODE_DOWN)) {
            changeSelection(1);
        }

        if (Input::justPressed(SDL_SCANCODE_TAB)) {
            if (!UserSession::IsLoggedIn()) {
                startTransitionOut(0.5f);
                Engine::getInstance()->switchState(new LoginState());
            }
        }

        if (Input::justPressed(SDL_SCANCODE_RETURN)) {
            switch (currentSelection) {
                case 0: // single player
                    startTransitionOut(0.5f);
                    Engine::getInstance()->switchState(new FreeplayState());
                    break;
                case 1: // online level download shit
                    startTransitionOut(0.5f);
                    Engine::getInstance()->switchState(new OnlineDLState());
                    break;
                case 2: // settigns
                    startTransitionOut(0.5f);
                    Engine::getInstance()->switchState(new OptionsState());
                    break;
                case 3: // quitting the game
                    Engine::getInstance()->quit();
                    break;
            }
        }
    }
}

void MainMenuState::render() {
    SDL_SetRenderDrawColor(SDLManager::getInstance().getRenderer(), 0, 0, 0, 255);
    
    if (titleSprite) {
        titleSprite->render();
    }

    if (usernameText) {
        usernameText->render();
    }
    
    for (auto item : menuItems) {
        if (item) {
            item->render();
        }
    }

    SwagState::render();
}

void MainMenuState::destroy() {
    titleSprite = nullptr;
    usernameText = nullptr;
    menuItems.clear();

    SwagState::destroy();
}

void MainMenuState::changeSelection(int change) {
    currentSelection += change;

    if (currentSelection < 0) {
        currentSelection = static_cast<int>(menuTexts.size() - 1);
    }
    if (currentSelection >= static_cast<int>(menuTexts.size())) {
        currentSelection = 0;
    }

    for (size_t i = 0; i < menuItems.size(); i++) {
        if (i == static_cast<size_t>(currentSelection)) {
            menuItems[i]->setFormat(Paths::font("vcr.ttf"), 38, 0xFFFF00FF);
        } else {
            menuItems[i]->setFormat(Paths::font("vcr.ttf"), 32, 0xFFFFFFFF);
        }
        
        Engine* engine = Engine::getInstance();
        menuItems[i]->setPosition(
            (engine->getWindowWidth() - menuItems[i]->getWidth()) / 2,
            engine->getWindowHeight() * 0.5f + (i * 60)
        );
    }
}
