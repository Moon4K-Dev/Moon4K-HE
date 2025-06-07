#pragma once

#include "../SwagState.h"
#include "../game/GameConfig.h"
#include "../../engine/graphics/Text.h"
#include "../../engine/core/SDLManager.h"
#include "../ui/StrumNote.h"
#include <vector>

struct SkinOption {
    Text* titleText;
    Text* valueText;
    float y;
    int id;
    std::vector<std::string> values;
    std::vector<std::string> folderNames;
    int currentValueIndex;

    SkinOption(const std::string& title, const std::vector<std::string>& possibleValues, float yPos, int optionId);
    void setSelected(bool selected);
    void updateValue(const std::string& newValue);
    void cycleValue(bool forward);
    std::string getCurrentValue() const;
};

class SkinOptionsState : public SwagState {
private:
    std::vector<SkinOption> options;
    std::vector<StrumNote*> previewNotes;
    int selectedIndex;
    GameConfig* config;
    float laneOffset = 112.0f;
    const int keyCount = 4;

    void changeSelection(int change);
    void handleInput();
    void updateOptionValues();
    void loadAvailableSkins();
    void drawGradientRect(SDL_Renderer* renderer, int x, int y, int w, int h, 
                         SDL_Color start, SDL_Color end, bool vertical = true);
    void createPreviewNotes();
    void updatePreviewNotes(const std::string& skinName);

public:
    SkinOptionsState();
    ~SkinOptionsState() override;

    void create() override;
    void update(float deltaTime) override;
    void render() override;
    void destroy() override;
}; 