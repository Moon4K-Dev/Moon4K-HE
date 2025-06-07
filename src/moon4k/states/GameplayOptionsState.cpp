#include "GameplayOptionsState.h"
#include "OptionsState.h"
#include "../../engine/core/Engine.h"
#include "../../engine/input/Input.h"
#include "../../engine/utils/Log.h"
#include "../../engine/utils/Paths.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

void GameplayOptionsState::drawGradientRect(SDL_Renderer* renderer, int x, int y, int w, int h, SDL_Color start, SDL_Color end, bool vertical) {
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

GameplayOption::GameplayOption(const std::string& title, const std::string& type, float yPos, int optionId,
                             float min, float max, float inc)
    : y(yPos), id(optionId), type(type), minValue(min), maxValue(max), increment(inc) {
    
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

void GameplayOption::setSelected(bool selected) {
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

void GameplayOption::updateValue(const std::string& newValue) {
    valueText->setText(newValue);
}

GameplayOptionsState::GameplayOptionsState() : selectedIndex(0) {
    config = GameConfig::getInstance();
}

GameplayOptionsState::~GameplayOptionsState() {
    destroy();
}

void GameplayOptionsState::create() {
    SwagState::create();
    Engine* engine = Engine::getInstance();
    
    float startY = engine->getWindowHeight() * 0.2f;
    float spacing = engine->getWindowHeight() * 0.15f;
    
    options.emplace_back("Downscroll", "bool", startY, 0);
    options.emplace_back("Ghost Tapping", "bool", startY + spacing, 1);
    options.emplace_back("Scroll Speed", "float", startY + spacing * 2, 2, 0.5f, 5.0f, 0.1f);
    options.emplace_back("Note Offset", "float", startY + spacing * 3, 3, -1000.0f, 1000.0f, 10.0f);
    
    for (auto& option : options) {
        engine->addText(option.titleText);
        engine->addText(option.valueText);
    }

    updateOptionValues();
    changeSelection(0);
    Discord::GetInstance().SetDetails("Changing some Gameplay Options...");
    Discord::GetInstance().Update();
}

void GameplayOptionsState::updateOptionValues() {
    for (auto& option : options) {
        std::string value;
        if (option.titleText->getText() == "Downscroll") {
            value = config->getDownscroll() ? "ON" : "OFF";
        }
        else if (option.titleText->getText() == "Ghost Tapping") {
            value = config->getGhostTapping() ? "ON" : "OFF";
        }
        else if (option.titleText->getText() == "Scroll Speed") {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(1) << config->getScrollSpeed();
            value = ss.str() + "x";
        }
        else if (option.titleText->getText() == "Note Offset") {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(0) << config->getSongOffset();
            value = ss.str() + "ms";
        }
        option.updateValue(value);

        Engine* engine = Engine::getInstance();
        float titleX = (engine->getWindowWidth() - option.titleText->getWidth()) * 0.3f;
        float valueX = (engine->getWindowWidth() - option.valueText->getWidth()) * 0.7f;
        
        option.titleText->setPosition(titleX, option.y);
        option.valueText->setPosition(valueX, option.y);
    }
}

void GameplayOptionsState::update(float deltaTime) {
    SwagState::update(deltaTime);
    Input::UpdateKeyStates();
    Input::UpdateControllerStates();
    
    handleInput();
}

void GameplayOptionsState::handleInput() {
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

        if (option.type == "bool") {
            if (Input::justPressed(SDL_SCANCODE_LEFT) || Input::justPressed(SDL_SCANCODE_RIGHT) || 
                Input::justPressed(SDL_SCANCODE_RETURN)) {
                if (option.titleText->getText() == "Downscroll") {
                    config->setDownscroll(!config->getDownscroll());
                    valueChanged = true;
                }
                else if (option.titleText->getText() == "Ghost Tapping") {
                    config->setGhostTapping(!config->getGhostTapping());
                    valueChanged = true;
                }
            }
        }
        else if (option.type == "float") {
            float change = 0.0f;
            if (Input::justPressed(SDL_SCANCODE_LEFT)) {
                change = -option.increment;
            }
            else if (Input::justPressed(SDL_SCANCODE_RIGHT)) {
                change = option.increment;
            }

            if (change != 0.0f) {
                if (option.titleText->getText() == "Scroll Speed") {
                    float newSpeed = std::clamp(config->getScrollSpeed() + change, 
                                              option.minValue, option.maxValue);
                    config->setScrollSpeed(newSpeed);
                    valueChanged = true;
                }
                else if (option.titleText->getText() == "Note Offset") {
                    float newOffset = std::clamp(config->getSongOffset() + change, 
                                               option.minValue, option.maxValue);
                    config->setSongOffset(newOffset);
                    valueChanged = true;
                }
            }
        }

        if (valueChanged) {
            updateOptionValues();
        }
    }
}

void GameplayOptionsState::render() {
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

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 30);
    SDL_RenderDrawLine(renderer, 0, 40, engine->getWindowWidth(), 40);
    SDL_RenderDrawLine(renderer, 0, engine->getWindowHeight() - 40, engine->getWindowWidth(), engine->getWindowHeight() - 40);
}

void GameplayOptionsState::changeSelection(int change) {
    selectedIndex = (selectedIndex + change) % static_cast<int>(options.size());
    if (selectedIndex < 0) selectedIndex = options.size() - 1;

    for (size_t i = 0; i < options.size(); i++) {
        options[i].setSelected(i == static_cast<size_t>(selectedIndex));
    }
}

void GameplayOptionsState::destroy() {
    Engine* engine = Engine::getInstance();
    options.clear();
} 