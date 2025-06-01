#pragma once

#include "../SwagState.h"
#include "../../engine/graphics/Text.h"
#include "../../engine/graphics/Sprite.h"
#include "../../engine/core/Engine.h"
#include "../../engine/input/Input.h"
#include "../../engine/audio/SoundManager.h"
#include "../../engine/utils/Paths.h"
#include <vector>
#include <string>
#include <filesystem>

struct AudioFile {
    std::string filename;
    std::string description;
    float tweenOffset;
    float targetOffset;
};

class FreeplayState : public SwagState {
public:
    static FreeplayState* instance;
    
    FreeplayState();
    ~FreeplayState() override;
    
    void create() override;
    void update(float deltaTime) override;
    void render() override;
    void destroy() override;

private:
    void loadAudioFiles();
    void updateSelection(int change = 0);
    void renderBackground();
    void updateTweens(float deltaTime);
    void rescanSongs();
    
    std::vector<AudioFile> audioFiles;
    std::vector<Text*> fileTexts;
    std::vector<Text*> descriptionTexts;
    int selectedIndex;
    
    Text* titleText;
    Text* subtitleText;
    Sprite* backgroundPattern;
    
    float itemHeight;
    float scrollOffset;
    SDL_Color bgColor;
    SDL_Color dotColor;
    
    const float TWEEN_SPEED = 8.0f;
    const float MAX_OFFSET = -30.0f;
}; 