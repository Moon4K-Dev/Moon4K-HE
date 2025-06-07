#pragma once

#include "../SwagState.h"
#include "../game/GameConfig.h"
#include "../../engine/graphics/Text.h"
#include "../../engine/core/SDLManager.h"
#include <vector>

struct OptionBox {
    Text* titleText;
    Text* descText;
    float y;
    int id;

    OptionBox(const std::string& title, const std::string& desc, float yPos, int optionId);
    void setSelected(bool selected);
};

class OptionsState : public SwagState {
private:
    std::vector<std::vector<std::string>> menuItems;
    std::vector<OptionBox> optionBoxes;
    int selectedIndex;

    void changeSelection(int change);
    void handleInput();
    void updateOptionBoxes(float deltaTime);
    void openGraphicsOptions();
    void openGameplayOptions();
    void openSkinOptions();
    void openControlsOptions();

public:
    OptionsState();
    ~OptionsState() override;

    void create() override;
    void update(float deltaTime) override;
    void render() override;
    void destroy() override;
};