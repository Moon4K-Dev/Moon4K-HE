#include "FreeplayState.h"
#include "MainMenuState.h"
#include "PlayState.h"
#include <SDL2/SDL2_gfxPrimitives.h>
#include <filesystem>
#include <algorithm>
#include <fstream>
#include "../backend/json.hpp"
#include <cmath>

using json = nlohmann::json;

FreeplayState* FreeplayState::instance = nullptr;

void SDLCALL postmix_callback(void* udata, Uint8* stream, int len) {
    FreeplayState* state = static_cast<FreeplayState*>(udata);
    if (state) {
        const Sint16* samples = reinterpret_cast<const Sint16*>(stream);
        int num_samples = len / sizeof(Sint16);
        
        for (int i = 0; i < state->AUDIO_BUFFER_SIZE && i < num_samples/2; i++) {
            float left = samples[i*2] / 32768.0f;
            float right = samples[i*2+1] / 32768.0f;
            float combined = (left + right) * 0.5f;
            float amplified = std::copysign(std::pow(std::abs(combined), 0.7f), combined) * 2.5f;
            state->audioData[i] = std::clamp(amplified, -1.0f, 1.0f);
        }
    }
}

FreeplayState::FreeplayState() 
    : selectedIndex(0)
    , titleText(nullptr)
    , subtitleText(nullptr)
    , backgroundPattern(nullptr)
    , itemHeight(80.0f)
    , scrollOffset(0.0f)
    , currentSound(nullptr) {
    instance = this;
    bgColor = {20, 20, 20, 255};
    dotColor = {100, 100, 150, 100};
    audioData.resize(AUDIO_BUFFER_SIZE, 0.0f);
    
    Mix_SetPostMix(postmix_callback, this);
}

FreeplayState::~FreeplayState() {
    Mix_SetPostMix(nullptr, nullptr);
    if (currentSound) {
        delete currentSound;
        currentSound = nullptr;
    }
    destroy();
}

void FreeplayState::create() {
    SwagState::create();
    
    Engine* engine = Engine::getInstance();
    
    sdl2vis::EmbeddedVisualizer::Config visConfig;
    visConfig.x = 20;
    visConfig.y = engine->getWindowHeight() - 180;
    visConfig.width = engine->getWindowWidth() * 0.35f - 40;
    visConfig.height = 160;
    visConfig.bar_count = 48;
    visConfig.background_color = {15, 15, 15, 200};
    visConfig.bar_color = {100, 100, 150, 255};
    visConfig.smoothing_factor = 0.15f;
    
    visualizer = std::make_unique<sdl2vis::EmbeddedVisualizer>(
        SDLManager::getInstance().getRenderer(),
        visConfig
    );

    titleText = new Text();
    titleText->setFormat(Paths::font("vcr.ttf"), 32, 0xFFFFFFFF);
    titleText->setText("The....Freeplay Zone!");
    titleText->setPosition(20, 20);
    engine->addText(titleText);
    
    subtitleText = new Text();
    subtitleText->setFormat(Paths::font("vcr.ttf"), 18, 0xFFFFFFFF);
    subtitleText->setText("Press R to rescan songs or Press Space to play a song's audio!");
    subtitleText->setPosition(20, 60);
    engine->addText(subtitleText);
    
    loadAudioFiles();
    
    float sidebarWidth = engine->getWindowWidth() * 0.4f;
    float sidebarX = engine->getWindowWidth() - sidebarWidth;
    
    float yPos = 100.0f;
    for (const auto& audio : audioFiles) {
        Text* fileText = new Text();
        fileText->setFormat(Paths::font("vcr.ttf"), 20, 0xFFFFFFFF);
        fileText->setText(audio.filename);
        fileText->setPosition(sidebarX + 20, yPos);
        engine->addText(fileText);
        fileTexts.push_back(fileText);
        
        Text* descText = new Text();
        descText->setFormat(Paths::font("vcr.ttf"), 16, 0xAAAAAA);
        descText->setText(audio.description);
        descText->setPosition(sidebarX + 40, yPos + 25);
        engine->addText(descText);
        descriptionTexts.push_back(descText);
        
        yPos += itemHeight;
    }
    
    updateSelection();

    Discord::GetInstance().SetDetails("Selecting a song...(in the Freeplay Zone!)");
    Discord::GetInstance().Update();
}

void FreeplayState::update(float deltaTime) {
    SwagState::update(deltaTime);
    Input::UpdateKeyStates();
    Input::UpdateControllerStates();
    
    updateVisualizer(deltaTime);
    
    if (!isTransitioning()) {
        if (Input::justPressed(SDL_SCANCODE_UP)) {
            updateSelection(-1);
        }
        if (Input::justPressed(SDL_SCANCODE_DOWN)) {
            updateSelection(1);
        }

        if (Input::justPressed(SDL_SCANCODE_SPACE)) {
            if (!audioFiles.empty() && selectedIndex >= 0 && selectedIndex < static_cast<int>(audioFiles.size())) {
                std::string songName = audioFiles[selectedIndex].filename;
                std::string audioPath = "assets/charts/" + songName + "/" + songName + ".ogg";
                
                SoundManager::getInstance().stopMusic();
                if (currentSound) {
                    currentSound->stop();
                    currentSound = nullptr;
                }
                
                currentSound = new Sound();
                if (currentSound->load(audioPath)) {
                    currentSound->setVolume(0.5f);
                    currentSound->play();
                } else {
                    delete currentSound;
                    currentSound = nullptr;
                    Log::getInstance().error("Failed to load audio: " + audioPath);
                }
            }
        }
        
        if (Input::justPressed(SDL_SCANCODE_R)) {
            rescanSongs();
        }

        if (Input::justPressed(SDL_SCANCODE_ESCAPE) || Input::justPressed(SDL_SCANCODE_BACKSPACE)) {
            Engine::getInstance()->switchState(new MainMenuState());
        }
        
        if (Input::justPressed(SDL_SCANCODE_RETURN)) {
            if (!audioFiles.empty() && selectedIndex >= 0 && selectedIndex < static_cast<int>(audioFiles.size())) {
                SoundManager::getInstance().stopMusic();
                startTransitionOut(0.5f);
                std::string selectedSong = audioFiles[selectedIndex].filename;
                Engine::getInstance()->switchState(new PlayState(selectedSong));
            }
        }
    }
    
    updateTweens(deltaTime);
}

void FreeplayState::render() {
    SDL_Renderer* renderer = SDLManager::getInstance().getRenderer();
    Engine* engine = Engine::getInstance();
    
    SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
    SDL_RenderClear(renderer);
    
    renderBackground();
    
    float sidebarWidth = engine->getWindowWidth() * 0.4f;
    if (!audioFiles.empty()) {
        for (size_t i = 0; i < audioFiles.size(); i++) {
            if (i != selectedIndex) {
                float yPos = 150.0f - scrollOffset + (i * itemHeight);
                SDL_Rect sidebarRect = {
                    static_cast<int>(engine->getWindowWidth() - sidebarWidth + audioFiles[i].tweenOffset),
                    static_cast<int>(yPos),
                    static_cast<int>(sidebarWidth),
                    static_cast<int>(itemHeight)
                };
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
                SDL_RenderFillRect(renderer, &sidebarRect);
            }
        }

        float selectedYPos = 150.0f - scrollOffset + (selectedIndex * itemHeight);
        SDL_Rect selectedRect = {
            static_cast<int>(engine->getWindowWidth() - sidebarWidth + audioFiles[selectedIndex].tweenOffset),
            static_cast<int>(selectedYPos),
            static_cast<int>(sidebarWidth),
            static_cast<int>(itemHeight)
        };
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
        SDL_RenderFillRect(renderer, &selectedRect);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 30);
        SDL_RenderFillRect(renderer, &selectedRect);
    }
    
    if (titleText) titleText->render();
    if (subtitleText) subtitleText->render();
    
    for (auto text : fileTexts) {
        text->render();
    }
    for (auto text : descriptionTexts) {
        text->render();
    }
    
    if (visualizer) {
        visualizer->render();
    }
    
    SwagState::render();
}

void FreeplayState::destroy() {
    SoundManager::getInstance().stopMusic();
    if (currentSound) {
        currentSound->stop();
        currentSound = nullptr;
    }
    fileTexts.clear();
    descriptionTexts.clear();
}

void FreeplayState::loadAudioFiles() {
    audioFiles.clear();
    std::string chartsDir = "assets/charts/";
    
    if (!std::filesystem::exists(chartsDir)) {
        Log::getInstance().warning("Charts directory does not exist: " + chartsDir);
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(chartsDir)) {
        if (entry.is_directory()) {
            std::string songName = entry.path().filename().string();
            
            std::string moonPath = chartsDir + songName + "/" + songName + ".moon";
            std::string configPath = chartsDir + songName + "/chartConfig.json";
            std::transform(moonPath.begin(), moonPath.end(), moonPath.begin(), ::tolower);
            
            std::string displayName = songName;
            std::string description = "";
            std::string artist = "";
            float bpm = 0.0f;
            
            try {
                std::ifstream configFile(configPath);
                if (configFile.is_open()) {
                    json config;
                    configFile >> config;
                    
                    if (config.contains("chartConfig")) {
                        auto& chartConfig = config["chartConfig"];
                        displayName = chartConfig.value("displayName", songName);
                        description = chartConfig.value("description", "");
                        artist = chartConfig.value("artist", "Unknown Artist");
                    }
                }

                std::ifstream moonFile(moonPath);
                if (moonFile.is_open()) {
                    json j;
                    moonFile >> j;
                    bpm = j.value("bpm", 0.0f);
                }

                std::string finalDescription = "Artist: " + artist;
                if (!description.empty()) {
                    finalDescription += " | " + description;
                }
                if (bpm > 0) {
                    finalDescription += " | BPM: " + std::to_string(static_cast<int>(bpm));
                }

                audioFiles.push_back({displayName, finalDescription, 0.0f, 0.0f});
                
            } catch (const std::exception& e) {
                Log::getInstance().error("Error parsing song JSON: " + std::string(e.what()));
                audioFiles.push_back({songName, "No song data available", 0.0f, 0.0f});
            }
        }
    }

    std::sort(audioFiles.begin(), audioFiles.end(), 
        [](const AudioFile& a, const AudioFile& b) {
            return a.filename < b.filename;
        });
}

void FreeplayState::updateTweens(float deltaTime) {
    for (size_t i = 0; i < audioFiles.size(); i++) {
        AudioFile& file = audioFiles[i];
        
        file.targetOffset = (i == static_cast<size_t>(selectedIndex)) ? MAX_OFFSET : 0.0f;
        
        float diff = file.targetOffset - file.tweenOffset;
        file.tweenOffset += diff * TWEEN_SPEED * deltaTime;
        
        if (i < fileTexts.size() && i < descriptionTexts.size()) {
            float sidebarWidth = Engine::getInstance()->getWindowWidth() * 0.4f;
            float sidebarX = Engine::getInstance()->getWindowWidth() - sidebarWidth;
            float yPos = 150.0f - scrollOffset + (i * itemHeight);
            
            fileTexts[i]->setPosition(sidebarX + 20 + file.tweenOffset, yPos);
            descriptionTexts[i]->setPosition(sidebarX + 40 + file.tweenOffset, yPos + 25);
        }
    }
}

void FreeplayState::updateSelection(int change) {
    SoundManager::getInstance().stopMusic();
    if (currentSound) {
        currentSound->stop();
        currentSound = nullptr;
    }

    selectedIndex += change;
    
    if (selectedIndex < 0) {
        selectedIndex = audioFiles.size() - 1;
    }
    if (selectedIndex >= static_cast<int>(audioFiles.size())) {
        selectedIndex = 0;
    }
    
    for (size_t i = 0; i < fileTexts.size(); i++) {
        SDL_Color color = (i == static_cast<size_t>(selectedIndex)) ? SDL_Color{255, 255, 255, 255} : SDL_Color{200, 200, 200, 200};
        fileTexts[i]->setAlpha(color.a);
        descriptionTexts[i]->setAlpha(170);
    }
    
    float selectedY = 150.0f + (selectedIndex * itemHeight) - scrollOffset;
    float windowHeight = static_cast<float>(Engine::getInstance()->getWindowHeight());
    
    if (selectedY < 150.0f) {
        scrollOffset += (selectedY - 150.0f);
    }
    if (selectedY + itemHeight > windowHeight - 50.0f) {
        scrollOffset += (selectedY + itemHeight) - (windowHeight - 50.0f);
    }
}

void FreeplayState::rescanSongs() {
    for (auto text : fileTexts) {
        delete text;
    }
    fileTexts.clear();
    
    for (auto text : descriptionTexts) {
        delete text;
    }
    descriptionTexts.clear();
    
    loadAudioFiles();
    
    Engine* engine = Engine::getInstance();
    float sidebarWidth = engine->getWindowWidth() * 0.4f;
    float sidebarX = engine->getWindowWidth() - sidebarWidth;
    float yPos = 150.0f;
    
    for (const auto& audio : audioFiles) {
        Text* fileText = new Text();
        fileText->setFormat(Paths::font("vcr.ttf"), 20, 0xFFFFFFFF);
        fileText->setText(audio.filename);
        fileText->setPosition(sidebarX + 20, yPos);
        engine->addText(fileText);
        fileTexts.push_back(fileText);
        
        Text* descText = new Text();
        descText->setFormat(Paths::font("vcr.ttf"), 16, 0xAAAAAA);
        descText->setText(audio.description);
        descText->setPosition(sidebarX + 40, yPos + 25);
        engine->addText(descText);
        descriptionTexts.push_back(descText);
        
        yPos += itemHeight;
    }
    
    selectedIndex = 0;
    updateSelection();
}

void FreeplayState::renderBackground() {
    SDL_Renderer* renderer = SDLManager::getInstance().getRenderer();
    int windowWidth = Engine::getInstance()->getWindowWidth();
    int windowHeight = Engine::getInstance()->getWindowHeight();
    
    int dotSpacing = 30;
    int dotSize = 4;
    
    for (int x = 0; x < windowWidth; x += dotSpacing) {
        for (int y = 0; y < windowHeight; y += dotSpacing) {
            filledCircleRGBA(renderer, x, y, dotSize, 
                           dotColor.r, dotColor.g, dotColor.b, dotColor.a);
        }
    }
}

void FreeplayState::updateVisualizer(float deltaTime) {
    if (visualizer) {
        visualizer->update_audio_data(audioData.data(), audioData.size());
    }
} 