#include "ControlsOptionsState.h"
#include "OptionsState.h"
#include "../../engine/core/Engine.h"
#include "../../engine/input/Input.h"
#include "../../engine/utils/Log.h"
#include "../../engine/utils/Paths.h"
#include <algorithm>
#include <fstream>
#include "../backend/json.hpp"

void ControlsOptionsState::drawGradientRect(SDL_Renderer* renderer, int x, int y, int w, int h, SDL_Color start, SDL_Color end, bool vertical) {
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

std::string ControlsOptionsState::getKeyName(SDL_Scancode key) const {
    const char* keyName = SDL_GetScancodeName(key);
    if (keyName && strlen(keyName) > 0) {
        std::string name = keyName;
        if (name == "Left") return "←";
        if (name == "Right") return "→";
        if (name == "Up") return "↑";
        if (name == "Down") return "↓";
        return name;
    }
    return "None";
}

KeybindOption::KeybindOption(const std::string& title, const std::string& actionName, float yPos, int optionId,
                           SDL_Scancode primary, SDL_Scancode alternate)
    : y(yPos), id(optionId), action(actionName), primaryKey(primary), alternateKey(alternate),
      isWaitingForInput(false), isAlternate(false) {
    
    titleText = new Text();
    titleText->setFormat(Paths::font("vcr.ttf"), 32, 0xFFFFFFFF);
    titleText->setText(title);
    titleText->setPosition(0, y);
    titleText->setAlpha(0.6f);

    primaryKeyText = new Text();
    primaryKeyText->setFormat(Paths::font("vcr.ttf"), 32, 0xFFFFFFFF);
    primaryKeyText->setPosition(0, y);
    primaryKeyText->setAlpha(0.6f);

    altKeyText = new Text();
    altKeyText->setFormat(Paths::font("vcr.ttf"), 32, 0xFFFFFFFF);
    altKeyText->setPosition(0, y);
    altKeyText->setAlpha(0.6f);

    updateKeyText();
}

void KeybindOption::setSelected(bool selected) {
    if (selected) {
        titleText->setFormat(Paths::font("vcr.ttf"), 42, 0xFFFF00FF);
        titleText->setAlpha(1.0f);
        
        if (isWaitingForInput) {
            primaryKeyText->setFormat(Paths::font("vcr.ttf"), 42, isWaitingForInput && !isAlternate ? 0xFF0000FF : 0xFFFF00FF);
            altKeyText->setFormat(Paths::font("vcr.ttf"), 42, isWaitingForInput && isAlternate ? 0xFF0000FF : 0xFFFF00FF);
        } else {
            primaryKeyText->setFormat(Paths::font("vcr.ttf"), 42, !isAlternate ? 0x00FF00FF : 0xFFFF00FF);
            altKeyText->setFormat(Paths::font("vcr.ttf"), 42, isAlternate ? 0x00FF00FF : 0xFFFF00FF);
        }
        
        primaryKeyText->setAlpha(1.0f);
        altKeyText->setAlpha(1.0f);
    } else {
        titleText->setFormat(Paths::font("vcr.ttf"), 32, 0xFFFFFFFF);
        titleText->setAlpha(0.6f);
        primaryKeyText->setFormat(Paths::font("vcr.ttf"), 32, 0xFFFFFFFF);
        primaryKeyText->setAlpha(0.6f);
        altKeyText->setFormat(Paths::font("vcr.ttf"), 32, 0xFFFFFFFF);
        altKeyText->setAlpha(0.6f);
    }
}

void KeybindOption::updateKeyText() {
    primaryKeyText->setText(getKeyName(primaryKey));
    altKeyText->setText(getKeyName(alternateKey));
}

std::string KeybindOption::getKeyName(SDL_Scancode key) const {
    const char* keyName = SDL_GetScancodeName(key);
    if (keyName && strlen(keyName) > 0) {
        std::string name = keyName;
        if (name == "Left") return "←";
        if (name == "Right") return "→";
        if (name == "Up") return "↑";
        if (name == "Down") return "↓";
        return name;
    }
    return "None";
}

ControlsOptionsState::ControlsOptionsState() : selectedIndex(0), isBinding(false) {
    config = GameConfig::getInstance();
}

ControlsOptionsState::~ControlsOptionsState() {
    destroy();
}

void ControlsOptionsState::loadCurrentKeybinds() {
    std::ifstream configFile("assets/config.json");
    if (!configFile.is_open()) {
        Log::getInstance().error("Failed to open config.json");
        return;
    }

    nlohmann::json config;
    try {
        configFile >> config;
    } catch (const std::exception& e) {
        Log::getInstance().error("Failed to parse config.json: " + std::string(e.what()));
        return;
    }

    auto getScancodeFromString = [](const std::string& keyName) -> SDL_Scancode {
        static const std::map<std::string, SDL_Scancode> keyMap = {
            {"A", SDL_SCANCODE_A}, {"B", SDL_SCANCODE_B}, {"C", SDL_SCANCODE_C},
            {"D", SDL_SCANCODE_D}, {"E", SDL_SCANCODE_E}, {"F", SDL_SCANCODE_F},
            {"G", SDL_SCANCODE_G}, {"H", SDL_SCANCODE_H}, {"I", SDL_SCANCODE_I},
            {"J", SDL_SCANCODE_J}, {"K", SDL_SCANCODE_K}, {"L", SDL_SCANCODE_L},
            {"M", SDL_SCANCODE_M}, {"N", SDL_SCANCODE_N}, {"O", SDL_SCANCODE_O},
            {"P", SDL_SCANCODE_P}, {"Q", SDL_SCANCODE_Q}, {"R", SDL_SCANCODE_R},
            {"S", SDL_SCANCODE_S}, {"T", SDL_SCANCODE_T}, {"U", SDL_SCANCODE_U},
            {"V", SDL_SCANCODE_V}, {"W", SDL_SCANCODE_W}, {"X", SDL_SCANCODE_X},
            {"Y", SDL_SCANCODE_Y}, {"Z", SDL_SCANCODE_Z},
            {"Left", SDL_SCANCODE_LEFT}, {"Right", SDL_SCANCODE_RIGHT},
            {"Up", SDL_SCANCODE_UP}, {"Down", SDL_SCANCODE_DOWN},
            {"Space", SDL_SCANCODE_SPACE}, {"Enter", SDL_SCANCODE_RETURN},
            {"Escape", SDL_SCANCODE_ESCAPE}
        };

        auto it = keyMap.find(keyName);
        return it != keyMap.end() ? it->second : SDL_SCANCODE_UNKNOWN;
    };

    Engine* engine = Engine::getInstance();
    float startY = engine->getWindowHeight() * 0.2f;
    float spacing = engine->getWindowHeight() * 0.15f;

    SDL_Scancode leftKey = SDL_SCANCODE_LEFT;
    SDL_Scancode downKey = SDL_SCANCODE_DOWN;
    SDL_Scancode upKey = SDL_SCANCODE_UP;
    SDL_Scancode rightKey = SDL_SCANCODE_RIGHT;

    SDL_Scancode altLeftKey = SDL_SCANCODE_A;
    SDL_Scancode altDownKey = SDL_SCANCODE_S;
    SDL_Scancode altUpKey = SDL_SCANCODE_W;
    SDL_Scancode altRightKey = SDL_SCANCODE_D;

    if (config.contains("mainBinds")) {
        auto mainBinds = config["mainBinds"];
        leftKey = getScancodeFromString(mainBinds["left"].get<std::string>());
        downKey = getScancodeFromString(mainBinds["down"].get<std::string>());
        upKey = getScancodeFromString(mainBinds["up"].get<std::string>());
        rightKey = getScancodeFromString(mainBinds["right"].get<std::string>());
    }

    if (config.contains("altBinds")) {
        auto altBinds = config["altBinds"];
        altLeftKey = getScancodeFromString(altBinds["left"].get<std::string>());
        altDownKey = getScancodeFromString(altBinds["down"].get<std::string>());
        altUpKey = getScancodeFromString(altBinds["up"].get<std::string>());
        altRightKey = getScancodeFromString(altBinds["right"].get<std::string>());
    }

    options.clear();
    options.emplace_back("Left Arrow", "left", startY, 0, leftKey, altLeftKey);
    options.emplace_back("Down Arrow", "down", startY + spacing, 1, downKey, altDownKey);
    options.emplace_back("Up Arrow", "up", startY + spacing * 2, 2, upKey, altUpKey);
    options.emplace_back("Right Arrow", "right", startY + spacing * 3, 3, rightKey, altRightKey);
}

void ControlsOptionsState::saveKeybinds() {
    std::ifstream inFile("assets/config.json");
    if (!inFile.is_open()) {
        Log::getInstance().error("Failed to open config.json for reading");
        return;
    }

    nlohmann::json config;
    try {
        inFile >> config;
    } catch (const std::exception& e) {
        Log::getInstance().error("Failed to parse config.json: " + std::string(e.what()));
        return;
    }
    inFile.close();

    auto getStringFromScancode = [](SDL_Scancode key) -> std::string {
        const char* keyName = SDL_GetScancodeName(key);
        if (keyName && strlen(keyName) > 0) {
            std::string name = keyName;
            if (name == "←") return "Left";
            if (name == "→") return "Right";
            if (name == "↑") return "Up";
            if (name == "↓") return "Down";
            return name;
        }
        return "Unknown";
    };

    nlohmann::json mainBinds;
    nlohmann::json altBinds;

    for (const auto& option : options) {
        mainBinds[option.action] = getStringFromScancode(option.primaryKey);
        altBinds[option.action] = getStringFromScancode(option.alternateKey);
    }

    config["mainBinds"] = mainBinds;
    config["altBinds"] = altBinds;

    std::ofstream outFile("assets/config.json");
    if (!outFile.is_open()) {
        Log::getInstance().error("Failed to open config.json for writing");
        return;
    }

    outFile << config.dump(4);
    outFile.close();
}

void ControlsOptionsState::create() {
    SwagState::create();
    loadCurrentKeybinds();
    
    Engine* engine = Engine::getInstance();
    for (auto& option : options) {
        engine->addText(option.titleText);
        engine->addText(option.primaryKeyText);
        engine->addText(option.altKeyText);
    }

    updateOptionValues();
    changeSelection(0);
    Discord::GetInstance().SetDetails("Changing my binds");
    Discord::GetInstance().Update();
}

void ControlsOptionsState::updateOptionValues() {
    Engine* engine = Engine::getInstance();
    for (auto& option : options) {
        float titleX = (engine->getWindowWidth() - option.titleText->getWidth()) * 0.3f;
        float primaryX = (engine->getWindowWidth() - option.primaryKeyText->getWidth()) * 0.6f;
        float altX = (engine->getWindowWidth() - option.altKeyText->getWidth()) * 0.8f;
        
        option.titleText->setPosition(titleX, option.y);
        option.primaryKeyText->setPosition(primaryX, option.y);
        option.altKeyText->setPosition(altX, option.y);
    }
}

void ControlsOptionsState::update(float deltaTime) {
    SwagState::update(deltaTime);
    Input::UpdateKeyStates();
    Input::UpdateControllerStates();
    
    if (isBinding) {
        auto& option = options[selectedIndex];
        
        for (int i = SDL_SCANCODE_A; i <= SDL_SCANCODE_Z; i++) {
            if (Input::justPressed(static_cast<SDL_Scancode>(i))) {
                if (option.isAlternate) {
                    option.alternateKey = static_cast<SDL_Scancode>(i);
                } else {
                    option.primaryKey = static_cast<SDL_Scancode>(i);
                }
                option.updateKeyText();
                option.isWaitingForInput = false;
                isBinding = false;
                updateOptionValues();
                saveKeybinds();
                return;
            }
        }

        SDL_Scancode arrowKeys[] = {
            SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, 
            SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT
        };
        for (auto key : arrowKeys) {
            if (Input::justPressed(key)) {
                if (option.isAlternate) {
                    option.alternateKey = key;
                } else {
                    option.primaryKey = key;
                }
                option.updateKeyText();
                option.isWaitingForInput = false;
                isBinding = false;
                updateOptionValues();
                saveKeybinds();
                return;
            }
        }

        if (Input::justPressed(SDL_SCANCODE_ESCAPE)) {
            option.isWaitingForInput = false;
            isBinding = false;
            updateOptionValues();
            return;
        }
    } else {
        handleInput();
    }
}

void ControlsOptionsState::handleInput() {
    if (isBinding) {
        const Uint8* state = SDL_GetKeyboardState(nullptr);
        for (int i = 0; i < SDL_NUM_SCANCODES; i++) {
            if (state[i]) {
                auto& option = options[selectedIndex];
                SDL_Scancode newKey = static_cast<SDL_Scancode>(i);
                
                if (newKey == SDL_SCANCODE_ESCAPE) {
                    option.isWaitingForInput = false;
                    isBinding = false;
                    updateOptionValues();
                    return;
                }

                if (option.isAlternate) {
                    option.alternateKey = newKey;
                } else {
                    option.primaryKey = newKey;
                }

                option.updateKeyText();
                option.isWaitingForInput = false;
                isBinding = false;
                updateOptionValues();
                saveKeybinds();
                return;
            }
        }
        return;
    }

    if (Input::justPressed(SDL_SCANCODE_ESCAPE) || Input::justPressed(SDL_SCANCODE_BACKSPACE)) {
        startTransitionOut(0.5f);
        Engine::getInstance()->switchState(new OptionsState());
        return;
    }

    if (!options.empty()) {
        int prevIndex = selectedIndex;
        bool wasAlternate = selectedIndex >= 0 && selectedIndex < static_cast<int>(options.size()) ? 
            options[selectedIndex].isAlternate : false;

        if (Input::justPressed(SDL_SCANCODE_UP)) {
            selectedIndex--;
            if (selectedIndex < 0) {
                selectedIndex = static_cast<int>(options.size()) - 1;
            }
        }
        if (Input::justPressed(SDL_SCANCODE_DOWN)) {
            selectedIndex++;
            if (selectedIndex >= static_cast<int>(options.size())) {
                selectedIndex = 0;
            }
        }

        if (prevIndex != selectedIndex && selectedIndex >= 0 && selectedIndex < static_cast<int>(options.size())) {
            options[selectedIndex].isAlternate = wasAlternate;
        }

        if (selectedIndex >= 0 && selectedIndex < static_cast<int>(options.size())) {
            auto& option = options[selectedIndex];
            
            if (Input::justPressed(SDL_SCANCODE_LEFT) || Input::justPressed(SDL_SCANCODE_RIGHT)) {
                option.isAlternate = !option.isAlternate;
            }

            if (Input::justPressed(SDL_SCANCODE_RETURN)) {
                isBinding = true;
                option.isWaitingForInput = true;
            }

            option.setSelected(true);
        }

        for (size_t i = 0; i < options.size(); i++) {
            if (i != static_cast<size_t>(selectedIndex)) {
                options[i].setSelected(false);
            }
        }
    }
}

void ControlsOptionsState::render() {
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

        float primaryX = (engine->getWindowWidth() - selectedOption.primaryKeyText->getWidth()) * 0.6f - 10;
        float altX = (engine->getWindowWidth() - selectedOption.altKeyText->getWidth()) * 0.8f - 10;
        float indicatorWidth = 120;
        float indicatorHeight = 40;

        SDL_Color activeColor = {0, 255, 0, 40};
        SDL_Color inactiveColor = {60, 60, 80, 40};

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 
            !selectedOption.isAlternate ? activeColor.r : inactiveColor.r,
            !selectedOption.isAlternate ? activeColor.g : inactiveColor.g,
            !selectedOption.isAlternate ? activeColor.b : inactiveColor.b,
            !selectedOption.isAlternate ? activeColor.a : inactiveColor.a);
        SDL_Rect primaryRect = {
            static_cast<int>(primaryX), 
            static_cast<int>(highlightY + 10), 
            static_cast<int>(indicatorWidth), 
            static_cast<int>(indicatorHeight)
        };
        SDL_RenderFillRect(renderer, &primaryRect);

        SDL_SetRenderDrawColor(renderer, 
            selectedOption.isAlternate ? activeColor.r : inactiveColor.r,
            selectedOption.isAlternate ? activeColor.g : inactiveColor.g,
            selectedOption.isAlternate ? activeColor.b : inactiveColor.b,
            selectedOption.isAlternate ? activeColor.a : inactiveColor.a);
        SDL_Rect altRect = {
            static_cast<int>(altX), 
            static_cast<int>(highlightY + 10), 
            static_cast<int>(indicatorWidth), 
            static_cast<int>(indicatorHeight)
        };
        SDL_RenderFillRect(renderer, &altRect);
    }

    SwagState::render();

    static float time = 0.0f;
    time += 0.016f;

    for (size_t i = 0; i < options.size(); i++) {
        const auto& option = options[i];
        if (option.titleText && option.primaryKeyText && option.altKeyText) {
            if (i != static_cast<size_t>(selectedIndex)) {
                float wave = std::sin(time * 2.0f + i * 0.5f) * 3.0f;
                option.titleText->setPosition(option.titleText->getX(), option.y + wave);
                option.primaryKeyText->setPosition(option.primaryKeyText->getX(), option.y + wave);
                option.altKeyText->setPosition(option.altKeyText->getX(), option.y + wave);
            }
            
            option.titleText->render();
            option.primaryKeyText->render();
            option.altKeyText->render();
        }
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 30);
    SDL_RenderDrawLine(renderer, 0, 40, engine->getWindowWidth(), 40);
    SDL_RenderDrawLine(renderer, 0, engine->getWindowHeight() - 40, engine->getWindowWidth(), engine->getWindowHeight() - 40);

    if (isBinding) {
        Text instructionText;
        instructionText.setFormat(Paths::font("vcr.ttf"), 32, 0xFFFFFFFF);
        instructionText.setText("Press any key to bind (ESC to cancel)");
        float centerX = (engine->getWindowWidth() - instructionText.getWidth()) / 2;
        instructionText.setPosition(centerX, engine->getWindowHeight() - 100);
        instructionText.render();
    } else {
        Text instructionText;
        instructionText.setFormat(Paths::font("vcr.ttf"), 24, 0xFFFFFFFF);
        instructionText.setText("ENTER to rebind | LEFT/RIGHT to switch primary/alt");
        float centerX = (engine->getWindowWidth() - instructionText.getWidth()) / 2;
        instructionText.setPosition(centerX, engine->getWindowHeight() - 100);
        instructionText.render();
    }
}

void ControlsOptionsState::changeSelection(int change) {
    if (isBinding) return;

    selectedIndex = (selectedIndex + change) % static_cast<int>(options.size());
    if (selectedIndex < 0) selectedIndex = options.size() - 1;

    for (size_t i = 0; i < options.size(); i++) {
        options[i].setSelected(i == static_cast<size_t>(selectedIndex));
    }
}

void ControlsOptionsState::destroy() {
    options.clear();
} 