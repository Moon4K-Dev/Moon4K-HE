#include "SkinOptionsState.h"
#include "OptionsState.h"
#include "../../engine/core/Engine.h"
#include "../../engine/input/Input.h"
#include "../../engine/utils/Log.h"
#include "../../engine/utils/Paths.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include "../backend/json.hpp"

void SkinOptionsState::drawGradientRect(SDL_Renderer* renderer, int x, int y, int w, int h, SDL_Color start, SDL_Color end, bool vertical) {
    for (int i = 0; i < (vertical ? h : w); i++) {
        float ratio = static_cast<float>(i) / (vertical ? h : w);
        Uint8 r = start.r + static_cast<Uint8>((end.r - start.r) * ratio);
        Uint8 g = start.g + static_cast<Uint8>((end.g - start.g) * ratio);
        Uint8 b = start.b + static_cast<Uint8>((end.b - start.b) * ratio);
        Uint8 a = start.a + static_cast<Uint8>((end.a - start.a) * ratio);
        SDL_SetRenderDrawColor(renderer, r, g, b, a);
        if (vertical) {
            SDL_RenderDrawLine(renderer, x, y + i, x + w, y + i);
        } else {
            SDL_RenderDrawLine(renderer, x + i, y, x + i, y + h);
        }
    }
}

SkinOption::SkinOption(const std::string& title, const std::vector<std::string>& possibleValues, float yPos, int optionId)
    : y(yPos), id(optionId), values(possibleValues), currentValueIndex(0) {
    
    titleText = new Text();
    titleText->setFormat(Paths::font("vcr.ttf"), 32, 0xFFFFFFFF);
    titleText->setText(title);
    titleText->setPosition(0, y);
    titleText->setAlpha(0.6f);

    valueText = new Text();
    valueText->setFormat(Paths::font("vcr.ttf"), 32, 0xFFFFFFFF);
    valueText->setPosition(0, y);
    valueText->setAlpha(0.6f);
}

void SkinOption::setSelected(bool selected) {
    if (selected) {
        titleText->setFormat(Paths::font("vcr.ttf"), 42, 0xFFFF00FF);
        titleText->setAlpha(1.0f);
        valueText->setFormat(Paths::font("vcr.ttf"), 42, 0xFFFF00FF);
        valueText->setAlpha(1.0f);
    } else {
        titleText->setFormat(Paths::font("vcr.ttf"), 32, 0xFFFFFFFF);
        titleText->setAlpha(0.6f);
        valueText->setFormat(Paths::font("vcr.ttf"), 32, 0xFFFFFFFF);
        valueText->setAlpha(0.6f);
    }
}

void SkinOption::updateValue(const std::string& newValue) {
    valueText->setText(newValue);
}

void SkinOption::cycleValue(bool forward) {
    if (values.empty()) return;
    
    if (forward) {
        currentValueIndex = (currentValueIndex + 1) % values.size();
    } else {
        currentValueIndex = (currentValueIndex - 1 + values.size()) % values.size();
    }
    updateValue(getCurrentValue());
}

std::string SkinOption::getCurrentValue() const {
    if (values.empty() || currentValueIndex >= values.size()) return "";
    return values[currentValueIndex];
}

void SkinOptionsState::createPreviewNotes() {
    Engine* engine = Engine::getInstance();
    float yPos = engine->getWindowHeight() * 0.6f;
    float totalWidth = laneOffset * (keyCount - 1);
    float startX = (engine->getWindowWidth() / 2.0f) - (totalWidth / 2.0f) - 45;
    
    for (auto note : previewNotes) {
        delete note;
    }
    previewNotes.clear();

    std::string currentSkin = options[0].getCurrentValue();
    for (int i = 0; i < keyCount; i++) {
        StrumNote* babyArrow = new StrumNote(0, yPos, i, currentSkin, keyCount);
        float x = startX + (i * laneOffset);
        babyArrow->setPosition(x, yPos);
        previewNotes.push_back(babyArrow);
    }
}

void SkinOptionsState::updatePreviewNotes(const std::string& skinName) {
    Engine* engine = Engine::getInstance();
    float yPos = engine->getWindowHeight() * 0.6f;
    float totalWidth = laneOffset * (keyCount - 1);
    float startX = (engine->getWindowWidth() / 2.0f) - (totalWidth / 2.0f) - 45;
    
    for (auto note : previewNotes) {
        delete note;
    }
    previewNotes.clear();

    for (int i = 0; i < keyCount; i++) {
        StrumNote* babyArrow = new StrumNote(0, yPos, i, skinName, keyCount);
        float x = startX + (i * laneOffset);
        babyArrow->setPosition(x, yPos);
        previewNotes.push_back(babyArrow);
        babyArrow->playAnimation("static");
    }
}

SkinOptionsState::SkinOptionsState() : selectedIndex(0) {
    config = GameConfig::getInstance();
}

SkinOptionsState::~SkinOptionsState() {
    destroy();
}

void SkinOptionsState::loadAvailableSkins() {
    std::vector<std::pair<std::string, std::string>> skinInfo;
    std::string skinsPath = "assets/images/ui-skins";
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(skinsPath)) {
            if (entry.is_directory()) {
                std::string folderName = entry.path().filename().string();
                std::string configPath = skinsPath + "/" + folderName + "/config.json";
                
                if (std::filesystem::exists(configPath)) {
                    std::ifstream configFile(configPath);
                    if (configFile.is_open()) {
                        nlohmann::json config;
                        try {
                            configFile >> config;
                            if (config.contains("name")) {
                                std::string displayName = config["name"];
                                skinInfo.push_back({folderName, displayName});
                            } else {
                                skinInfo.push_back({folderName, folderName});
                            }
                        } catch (const nlohmann::json::exception& e) {
                            Log::getInstance().error("Error parsing config.json for skin " + folderName + ": " + e.what());
                            skinInfo.push_back({folderName, folderName});
                        }
                        configFile.close();
                    } else {
                        skinInfo.push_back({folderName, folderName});
                    }
                } else {
                    skinInfo.push_back({folderName, folderName});
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        Log::getInstance().error("Error loading skins: " + std::string(e.what()));
    }

    std::sort(skinInfo.begin(), skinInfo.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });
    
    std::vector<std::string> folderNames;
    std::vector<std::string> displayNames;
    for (const auto& info : skinInfo) {
        folderNames.push_back(info.first);
        displayNames.push_back(info.second);
    }
    
    std::string currentSkin = config->getNoteskin();
    auto it = std::find(folderNames.begin(), folderNames.end(), currentSkin);
    int currentIndex = (it != folderNames.end()) ? std::distance(folderNames.begin(), it) : 0;

    float startY = Engine::getInstance()->getWindowHeight() * 0.2f;
    options.emplace_back("UI Skin", displayNames, startY, 0);
    options.back().currentValueIndex = currentIndex;
    options.back().folderNames = folderNames;
}

void SkinOptionsState::create() {
    SwagState::create();
    loadAvailableSkins();
    
    Engine* engine = Engine::getInstance();
    for (auto& option : options) {
        engine->addText(option.titleText);
        engine->addText(option.valueText);
    }

    createPreviewNotes();
    updateOptionValues();
    changeSelection(0);

    Discord::GetInstance().SetDetails("Changing my NoteSkin...");
    Discord::GetInstance().Update();
}

void SkinOptionsState::updateOptionValues() {
    Engine* engine = Engine::getInstance();
    for (auto& option : options) {
        option.updateValue(option.getCurrentValue());

        float titleX = (engine->getWindowWidth() - option.titleText->getWidth()) * 0.3f;
        float valueX = (engine->getWindowWidth() - option.valueText->getWidth()) * 0.7f;
        
        option.titleText->setPosition(titleX, option.y);
        option.valueText->setPosition(valueX, option.y);
    }
}

void SkinOptionsState::update(float deltaTime) {
    SwagState::update(deltaTime);
    Input::UpdateKeyStates();
    Input::UpdateControllerStates();
    
    handleInput();

    for (auto note : previewNotes) {
        if (note) {
            note->update(deltaTime);
        }
    }
}

void SkinOptionsState::handleInput() {
    if (Input::justPressed(SDL_SCANCODE_ESCAPE) || Input::justPressed(SDL_SCANCODE_BACKSPACE)) {
        startTransitionOut(0.5f);
        Engine::getInstance()->switchState(new OptionsState());
        return;
    }

    if (Input::justPressed(SDL_SCANCODE_UP)) {
        changeSelection(-1);
    }
    if (Input::justPressed(SDL_SCANCODE_DOWN)) {
        changeSelection(1);
    }

    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(options.size())) {
        auto& option = options[selectedIndex];
        bool valueChanged = false;

        if (Input::justPressed(SDL_SCANCODE_LEFT)) {
            option.cycleValue(false);
            valueChanged = true;
        }
        else if (Input::justPressed(SDL_SCANCODE_RIGHT) || Input::justPressed(SDL_SCANCODE_RETURN)) {
            option.cycleValue(true);
            valueChanged = true;
        }

        if (valueChanged) {
            std::string folderName = option.folderNames[option.currentValueIndex];
            config->setNoteskin(folderName);
            updatePreviewNotes(folderName);
            updateOptionValues();

            for (auto note : previewNotes) {
                if (note) {
                    note->playAnimation("confirm");
                }
            }
        }
    }

    static bool isAnimating = false;
    static float animTimer = 0.0f;
    
    if (Input::justPressed(SDL_SCANCODE_SPACE)) {
        for (auto note : previewNotes) {
            if (note) {
                note->playAnimation("confirm");
            }
        }
        isAnimating = true;
        animTimer = 0.0f;
    }

    if (isAnimating) {
        animTimer += 0.016f;
        if (animTimer >= 0.15f) {
            for (auto note : previewNotes) {
                if (note) {
                    note->playAnimation("static");
                }
            }
            isAnimating = false;
        }
    }
}

void SkinOptionsState::render() {
    SDL_Renderer* renderer = SDLManager::getInstance().getRenderer();
    Engine* engine = Engine::getInstance();
    
    SDL_Color topColor = {20, 20, 30, 255};
    SDL_Color bottomColor = {40, 40, 60, 255};
    drawGradientRect(renderer, 0, 0, engine->getWindowWidth(), engine->getWindowHeight(), topColor, bottomColor, true);

    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(options.size())) {
        const auto& selectedOption = options[selectedIndex];
        float highlightY = selectedOption.y - 10;
        float highlightHeight = 60;
        
        SDL_Color highlightStart = {60, 60, 80, 100};
        SDL_Color highlightEnd = {40, 40, 60, 0};
        
        drawGradientRect(renderer, 0, highlightY, engine->getWindowWidth() / 2, highlightHeight,
                        highlightEnd, highlightStart, false);
        drawGradientRect(renderer, engine->getWindowWidth() / 2, highlightY, engine->getWindowWidth() / 2, highlightHeight,
                        highlightStart, highlightEnd, false);
    }

    SwagState::render();

    static float time = 0.0f;
    time += 0.016f;

    for (size_t i = 0; i < options.size(); i++) {
        const auto& option = options[i];
        if (option.titleText && option.valueText) {
            if (i != static_cast<size_t>(selectedIndex)) {
                float wave = std::sin(time * 2.0f + i * 0.5f) * 3.0f;
                option.titleText->setPosition(option.titleText->getX(), option.y + wave);
                option.valueText->setPosition(option.valueText->getX(), option.y + wave);
            }
            
            option.titleText->render();
            option.valueText->render();
        }
    }

    for (auto note : previewNotes) {
        if (note && note->isVisible()) {
            note->render();
        }
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 30);
    SDL_RenderDrawLine(renderer, 0, 40, engine->getWindowWidth(), 40);
    SDL_RenderDrawLine(renderer, 0, engine->getWindowHeight() - 40, engine->getWindowWidth(), engine->getWindowHeight() - 40);
}

void SkinOptionsState::changeSelection(int change) {
    selectedIndex = (selectedIndex + change) % static_cast<int>(options.size());
    if (selectedIndex < 0) selectedIndex = options.size() - 1;

    for (size_t i = 0; i < options.size(); i++) {
        options[i].setSelected(i == static_cast<size_t>(selectedIndex));
    }
}

void SkinOptionsState::destroy() {
    for (auto note : previewNotes) {
        delete note;
    }
    previewNotes.clear();
    options.clear();
} 