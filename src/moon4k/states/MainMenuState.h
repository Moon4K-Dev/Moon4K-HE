#pragma once

#include "../SwagState.h"
#include "../../engine/graphics/Sprite.h"
#include "../../engine/graphics/Text.h"
#include <vector>
#include <string>
#include <steam/steam_api.h>

class MainMenuState : public SwagState {
public:
    MainMenuState();
    ~MainMenuState();

    void create() override;
    void update(float deltaTime) override;
    void render() override;
    void destroy() override;

private:
    void changeSelection(int change = 0);

    Sprite* titleSprite;
    Sprite* avatarSprite;
    Sprite* avatarBorderSprite;
    Text* userNameText;
    std::vector<Text*> menuItems;
    std::vector<std::string> menuTexts;
    int currentSelection;
};
