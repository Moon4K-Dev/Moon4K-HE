#include "LoginState.h"
#include "../../engine/core/Engine.h"
#include "../../engine/input/Input.h"
#include "../../engine/core/SDLManager.h"
#include "../../engine/utils/Paths.h"
#include "MainMenuState.h"
#include "../network/NetworkManager.h"
#include "../network/UserSession.h"
#include <sstream>
#include "../backend/json.hpp"

using json = nlohmann::json;

LoginState::LoginState()
    : logoSprite(nullptr)
    , titleText(nullptr)
    , usernameText(nullptr)
    , messageText(nullptr)
    , currentSelection(0)
    , isTypingUsername(false)
    , currentMode(LoginMode::Login) {
}

LoginState::~LoginState() {
    destroy();
}

void LoginState::create() {
    SwagState::create();

    Engine* engine = Engine::getInstance();
    
    SDL_SetRenderDrawColor(SDLManager::getInstance().getRenderer(), 0, 0, 0, 255);

    logoSprite = new Sprite(Paths::image("sexylogobyhiro"));
    logoSprite->setScale(0.25f, 0.25f);
    logoSprite->setPosition(
        (engine->getWindowWidth() - logoSprite->getWidth() * 0.25f) / 2,
        50
    );
    engine->addSprite(logoSprite);

    titleText = new Text();
    titleText->setFormat(Paths::font("vcr.ttf"), 32, 0xFFFFFFFF);
    titleText->setText(currentMode == LoginMode::Login ? "Login" : "Register");
    titleText->setPosition(
        (engine->getWindowWidth() - titleText->getWidth()) / 2,
        logoSprite->getY() + logoSprite->getHeight() * 0.25f + 30
    );
    engine->addText(titleText);

    float startY = titleText->getY() + 60;

    usernameText = new Text();
    usernameText->setFormat(Paths::font("vcr.ttf"), 24, 0xFFFFFFFF);
    usernameText->setText("Username: " + username);
    usernameText->setPosition(
        (engine->getWindowWidth() - usernameText->getWidth()) / 2,
        startY
    );
    engine->addText(usernameText);

    messageText = new Text();
    messageText->setFormat(Paths::font("vcr.ttf"), 20, 0xFF0000FF);
    messageText->setText("");
    messageText->setPosition(
        (engine->getWindowWidth() - messageText->getWidth()) / 2,
        startY + 40
    );
    engine->addText(messageText);

    std::vector<std::string> options = {
        "Submit",
        currentMode == LoginMode::Login ? "Create Account" : "Back to Login",
        "Exit"
    };

    float buttonStartY = startY + 80;
    for (size_t i = 0; i < options.size(); i++) {
        Text* txt = new Text();
        txt->setFormat(Paths::font("vcr.ttf"), 24, 0xFFFFFFFF);
        txt->setText(options[i]);
        txt->setPosition(
            (engine->getWindowWidth() - txt->getWidth()) / 2,
            buttonStartY + (i * 40)
        );
        optionTexts.push_back(txt);
        engine->addText(txt);
    }

    changeSelection();
}

void LoginState::update(float deltaTime) {
    SwagState::update(deltaTime);

    if (!isTransitioning()) {
        Input::UpdateKeyStates();

        if (Input::justPressed(SDL_SCANCODE_TAB)) {
            isTypingUsername = !isTypingUsername;
            return;
        }
        
        if (Input::justPressed(SDL_SCANCODE_ESCAPE)) {
            isTypingUsername = false;
            return;
        }

        if (!isTypingUsername) {
            if (Input::justPressed(SDL_SCANCODE_UP)) {
                changeSelection(-1);
            }
            if (Input::justPressed(SDL_SCANCODE_DOWN)) {
                changeSelection(1);
            }
            if (Input::justPressed(SDL_SCANCODE_RETURN)) {
                if (currentSelection == 0) {
                    if (username.empty()) {
                        messageText->setText("Please enter a username");
                        messageText->setPosition(
                            (Engine::getInstance()->getWindowWidth() - messageText->getWidth()) / 2,
                            messageText->getY()
                        );
                    } else {
                        if (currentMode == LoginMode::Login) {
                            tryLogin();
                        } else {
                            tryRegister();
                        }
                    }
                } else if (currentSelection == 1) {
                    currentMode = currentMode == LoginMode::Login ? LoginMode::Register : LoginMode::Login;
                    titleText->setText(currentMode == LoginMode::Login ? "Login" : "Register");
                    optionTexts[1]->setText(currentMode == LoginMode::Login ? "Create Account" : "Back to Login");
                    messageText->setText("");
                    
                    titleText->setPosition(
                        (Engine::getInstance()->getWindowWidth() - titleText->getWidth()) / 2,
                        titleText->getY()
                    );
                    optionTexts[1]->setPosition(
                        (Engine::getInstance()->getWindowWidth() - optionTexts[1]->getWidth()) / 2,
                        optionTexts[1]->getY()
                    );
                } else if (currentSelection == 2) {
                    Engine::getInstance()->quit();
                }
            }
        }

        if (isTypingUsername) {
            if (Input::justPressed(SDL_SCANCODE_BACKSPACE)) {
                if (!username.empty()) {
                    username.pop_back();
                    usernameText->setText("Username: " + username);
                    usernameText->setPosition(
                        (Engine::getInstance()->getWindowWidth() - usernameText->getWidth()) / 2,
                        usernameText->getY()
                    );
                }
                return;
            }

            for (int i = SDL_SCANCODE_A; i <= SDL_SCANCODE_Z; i++) {
                if (Input::justPressed(static_cast<SDL_Scancode>(i))) {
                    handleInput(static_cast<SDL_Scancode>(i));
                }
            }
            for (int i = SDL_SCANCODE_0; i <= SDL_SCANCODE_9; i++) {
                if (Input::justPressed(static_cast<SDL_Scancode>(i))) {
                    handleInput(static_cast<SDL_Scancode>(i));
                }
            }
        }
    }
}

void LoginState::handleInput(SDL_Scancode key) {
    char c = '\0';
    if (key >= SDL_SCANCODE_A && key <= SDL_SCANCODE_Z) {
        c = 'a' + (key - SDL_SCANCODE_A);
    } else if (key >= SDL_SCANCODE_0 && key <= SDL_SCANCODE_9) {
        c = '0' + (key - SDL_SCANCODE_0);
    }
    
    if (c != '\0') {
        username += c;
        usernameText->setText("Username: " + username);
        usernameText->setPosition(
            (Engine::getInstance()->getWindowWidth() - usernameText->getWidth()) / 2,
            usernameText->getY()
        );
    }
}

void LoginState::tryLogin() {
    if (!NetworkManager::GetInstance().IsConnected()) {
        messageText->setText("Not connected to server");
        messageText->setPosition(
            (Engine::getInstance()->getWindowWidth() - messageText->getWidth()) / 2,
            messageText->getY()
        );
        return;
    }

    json j;
    j["Username"] = username;
    std::string postData = j.dump();
    std::string response;

    if (!NetworkManager::GetInstance().SendRequest("/login", postData, response)) {
        messageText->setText(NetworkManager::GetInstance().GetLastError());
        messageText->setPosition(
            (Engine::getInstance()->getWindowWidth() - messageText->getWidth()) / 2,
            messageText->getY()
        );
    } else {
        try {
            json responseJson = json::parse(response);
            if (responseJson.contains("status") && responseJson["status"] == "success") {
                UserSession::SetUsername(username);
                startTransitionOut(0.5f);
                Engine::getInstance()->switchState(new MainMenuState());
            } else if (responseJson.contains("error")) {
                messageText->setText(responseJson["error"].get<std::string>());
                messageText->setPosition(
                    (Engine::getInstance()->getWindowWidth() - messageText->getWidth()) / 2,
                    messageText->getY()
                );
            } else {
                messageText->setText("Invalid server response format");
                messageText->setPosition(
                    (Engine::getInstance()->getWindowWidth() - messageText->getWidth()) / 2,
                    messageText->getY()
                );
            }
        } catch (const json::parse_error& e) {
            messageText->setText("Invalid server response");
            messageText->setPosition(
                (Engine::getInstance()->getWindowWidth() - messageText->getWidth()) / 2,
                messageText->getY()
            );
        }
    }
}

void LoginState::tryRegister() {
    if (!NetworkManager::GetInstance().IsConnected()) {
        messageText->setText("Not connected to server");
        messageText->setPosition(
            (Engine::getInstance()->getWindowWidth() - messageText->getWidth()) / 2,
            messageText->getY()
        );
        return;
    }

    json j;
    j["Username"] = username;
    std::string postData = j.dump();
    std::string response;

    if (!NetworkManager::GetInstance().SendRequest("/register", postData, response)) {
        messageText->setText(NetworkManager::GetInstance().GetLastError());
        messageText->setPosition(
            (Engine::getInstance()->getWindowWidth() - messageText->getWidth()) / 2,
            messageText->getY()
        );
    } else {
        try {
            json responseJson = json::parse(response);
            if (responseJson.contains("status") && responseJson["status"] == "success") {
                messageText->setText("Registration successful! Please login.");
                currentMode = LoginMode::Login;
                titleText->setText("Login");
                optionTexts[1]->setText("Create Account");
                username.clear();
                usernameText->setText("Username: ");
                
                titleText->setPosition(
                    (Engine::getInstance()->getWindowWidth() - titleText->getWidth()) / 2,
                    titleText->getY()
                );
                optionTexts[1]->setPosition(
                    (Engine::getInstance()->getWindowWidth() - optionTexts[1]->getWidth()) / 2,
                    optionTexts[1]->getY()
                );
                usernameText->setPosition(
                    (Engine::getInstance()->getWindowWidth() - usernameText->getWidth()) / 2,
                    usernameText->getY()
                );
            } else if (responseJson.contains("error")) {
                messageText->setText(responseJson["error"].get<std::string>());
            } else {
                messageText->setText("Invalid server response format");
            }
            messageText->setPosition(
                (Engine::getInstance()->getWindowWidth() - messageText->getWidth()) / 2,
                messageText->getY()
            );
        } catch (const json::parse_error& e) {
            messageText->setText("Invalid server response");
            messageText->setPosition(
                (Engine::getInstance()->getWindowWidth() - messageText->getWidth()) / 2,
                messageText->getY()
            );
        }
    }
}

void LoginState::render() {
    SDL_SetRenderDrawColor(SDLManager::getInstance().getRenderer(), 0, 0, 0, 255);
    
    if (logoSprite) {
        logoSprite->render();
    }
    
    if (titleText) {
        titleText->render();
    }
    
    if (usernameText) {
        usernameText->render();
    }
    
    if (messageText) {
        messageText->render();
    }
    
    for (auto text : optionTexts) {
        if (text) {
            text->render();
        }
    }

    SwagState::render();
}

void LoginState::destroy() {
    logoSprite = nullptr;
    titleText = nullptr;
    usernameText = nullptr;
    messageText = nullptr;
    optionTexts.clear();

    SwagState::destroy();
}

void LoginState::changeSelection(int change) {
    currentSelection += change;

    if (currentSelection < 0) {
        currentSelection = optionTexts.size() - 1;
    }
    if (currentSelection >= static_cast<int>(optionTexts.size())) {
        currentSelection = 0;
    }

    for (size_t i = 0; i < optionTexts.size(); i++) {
        if (i == static_cast<size_t>(currentSelection)) {
            optionTexts[i]->setFormat(Paths::font("vcr.ttf"), 28, 0xFFFF00FF);
        } else {
            optionTexts[i]->setFormat(Paths::font("vcr.ttf"), 24, 0xFFFFFFFF);
        }
        
        optionTexts[i]->setPosition(
            (Engine::getInstance()->getWindowWidth() - optionTexts[i]->getWidth()) / 2,
            optionTexts[i]->getY()
        );
    }
} 