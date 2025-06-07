#pragma once

#include "../SwagState.h"
#include "../game/GameConfig.h"
#include "../../engine/graphics/Text.h"
#include "../../engine/core/SDLManager.h"
#include <vector>
#include <string>

struct KeybindOption {
    Text* titleText;
    Text* primaryKeyText;
    Text* altKeyText;
    float y;
    int id;
    std::string action;
    SDL_Scancode primaryKey;
    SDL_Scancode alternateKey;
    bool isWaitingForInput;
    bool isAlternate;

    KeybindOption(const std::string& title, const std::string& action, float yPos, int optionId,
                 SDL_Scancode primary = SDL_SCANCODE_UNKNOWN, SDL_Scancode alternate = SDL_SCANCODE_UNKNOWN);
    void setSelected(bool selected);
    void updateKeyText();
    std::string getKeyName(SDL_Scancode key) const;
};

class ControlsOptionsState : public SwagState {
private:
    std::vector<KeybindOption> options;
    int selectedIndex;
    GameConfig* config;
    bool isBinding;

    void changeSelection(int change);
    void handleInput();
    void updateOptionValues();
    void loadCurrentKeybinds();
    void saveKeybinds();
    void drawGradientRect(SDL_Renderer* renderer, int x, int y, int w, int h, 
                         SDL_Color start, SDL_Color end, bool vertical = true);
    std::string getKeyName(SDL_Scancode key) const;

public:
    ControlsOptionsState();
    ~ControlsOptionsState() override;

    void create() override;
    void update(float deltaTime) override;
    void render() override;
    void destroy() override;
}; 