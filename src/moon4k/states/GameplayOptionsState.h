#pragma once

#include "../SwagState.h"
#include "../game/GameConfig.h"
#include "../../engine/graphics/Text.h"
#include "../../engine/core/SDLManager.h"
#include <vector>

struct GameplayOption {
    Text* titleText;
    Text* valueText;
    float y;
    int id;
    std::string type;
    float minValue;
    float maxValue;
    float increment;

    GameplayOption(const std::string& title, const std::string& type, float yPos, int optionId,
                  float min = 0.0f, float max = 1.0f, float inc = 0.1f);
    void setSelected(bool selected);
    void updateValue(const std::string& newValue);
};

class GameplayOptionsState : public SwagState {
private:
    std::vector<GameplayOption> options;
    int selectedIndex;
    GameConfig* config;

    void changeSelection(int change);
    void handleInput();
    void updateOptionValues();
    void saveSettings();
    void drawGradientRect(SDL_Renderer* renderer, int x, int y, int w, int h, 
                         SDL_Color start, SDL_Color end, bool vertical = true);

public:
    GameplayOptionsState();
    ~GameplayOptionsState() override;

    void create() override;
    void update(float deltaTime) override;
    void render() override;
    void destroy() override;
}; 