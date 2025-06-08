#pragma once

#include "../SwagState.h"
#include "../../engine/graphics/Text.h"
#include "../../engine/graphics/Sprite.h"
#include "../../engine/core/Engine.h"
#include "../../engine/input/Input.h"
#include "../../engine/audio/SoundManager.h"
#include "../../engine/utils/Paths.h"
#include "../game/ScoreManager.h"
#include "../../../external/sdl2.vis/embedded_visualizer.hpp"
#include <vector>
#include <string>
#include <filesystem>

void SDLCALL postmix_callback(void* udata, Uint8* stream, int len);

struct AudioFile {
    std::string filename;
    std::string displayName;
    std::string description;
    float tweenOffset;
    float targetOffset;
    std::string format;
    std::vector<std::string> difficulties;
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
    friend void SDLCALL postmix_callback(void* udata, Uint8* stream, int len);

    void loadAudioFiles();
    void updateSelection(int change = 0);
    void renderBackground();
    void updateTweens(float deltaTime);
    void rescanSongs();
    void updateVisualizer(float deltaTime);
    std::string wrapText(const std::string& text, float maxWidth, int fontSize);
    void updateDifficultySelection();
    static std::string trim(const std::string& str);
    
    std::vector<AudioFile> audioFiles;
    std::vector<Text*> fileTexts;
    std::vector<Text*> descriptionTexts;
    int selectedIndex;
    int selectedDifficulty;
    float difficultyPanelOffset;
    bool showingDifficulties;
    
    Text* titleText;
    Text* subtitleText;
    Text* scoreText;
    Text* accuracyText;
    Text* missesText;
    Text* rankText;
    Sprite* backgroundPattern;
    
    float itemHeight;
    float scrollOffset;
    SDL_Color bgColor;
    SDL_Color dotColor;
    
    const float TWEEN_SPEED = 8.0f;
    const float MAX_OFFSET = -30.0f;

    std::unique_ptr<sdl2vis::EmbeddedVisualizer> visualizer;
    std::vector<float> audioData;
    static const size_t AUDIO_BUFFER_SIZE = 1024;
    Sound* currentSound;
}; 