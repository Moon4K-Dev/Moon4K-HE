#pragma once

#include "../../engine/core/SubState.h"
#include "../../engine/graphics/Sprite.h"
#include "../../engine/graphics/Text.h"
#include <steam/steam_api.h>
#include <vector>
#include <string>

class ProfileSubState : public SubState {
public:
    ProfileSubState();
    ~ProfileSubState();

    void create() override;
    void update(float deltaTime) override;
    void render() override;
    void destroy() override;

private:
    Sprite* bgOverlay;
    Sprite* avatarSprite;
    Sprite* avatarBorderSprite;
    Text* userNameText;
    Text* steamIDText;
    Text* statusText;
    
    std::vector<Text*> achievementTexts;
    std::vector<Text*> statTexts;
    
    Sprite* closeButtonSprite;
    bool isCloseHovered;
    
    void loadSteamProfile();
    void loadAchievements();
    void loadStats();
    void createCloseButton();
}; 