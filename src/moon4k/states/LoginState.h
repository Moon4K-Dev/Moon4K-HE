#pragma once

#include "../SwagState.h"
#include "../../engine/graphics/Sprite.h"
#include "../../engine/graphics/Text.h"
#include <vector>
#include <string>
#include "../../engine/core/SDLManager.h"

class LoginState : public SwagState {
public:
    LoginState();
    ~LoginState();

    void create() override;
    void update(float deltaTime) override;
    void render() override;
    void destroy() override;

private:
    enum class LoginMode {
        Login,
        Register
    };

    void handleInput(SDL_Scancode key);
    void changeSelection(int change = 0);
    void tryLogin();
    void tryRegister();

    Sprite* logoSprite;
    Text* titleText;
    Text* usernameText;
    Text* messageText;
    std::vector<Text*> optionTexts;

    std::string username;
    int currentSelection;
    bool isTypingUsername;
    LoginMode currentMode;
}; 