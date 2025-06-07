#include "OptionsState.h"
#include "MainMenuState.h"
#include "../../engine/core/Engine.h"
#include "../../engine/input/Input.h"
#include "../../engine/utils/Log.h"
#include "../../engine/utils/Paths.h"
#include <algorithm>
#include "GameplayOptionsState.h"
#include "SkinOptionsState.h"
#include "ControlsOptionsState.h"

void drawGradientRect(SDL_Renderer* renderer, int x, int y, int w, int h, SDL_Color start, SDL_Color end, bool vertical = true) {
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

OptionBox::OptionBox(const std::string& title, const std::string& desc, float yPos, int optionId) 
    : y(yPos), id(optionId) {
    titleText = new Text();
    titleText->setFormat(Paths::font("vcr.ttf"), 32, 0xFFFFFFFF);
    titleText->setText(title);
    titleText->setPosition(0, y);

    descText = new Text();
    descText->setFormat(Paths::font("vcr.ttf"), 16, 0xFFFFFFFF);
    descText->setText(desc);
    descText->setPosition(0, y + 40);
    descText->setAlpha(0.7f);
}

void OptionBox::setSelected(bool selected) {
    if (selected) {
        titleText->setFormat(Paths::font("vcr.ttf"), 42, 0xFFFF00FF);
        titleText->setAlpha(1.0f);
        descText->setFormat(Paths::font("vcr.ttf"), 20, 0xFFFFFFFF);
        descText->setAlpha(0.9f);
    } else {
        titleText->setFormat(Paths::font("vcr.ttf"), 32, 0xFFFFFFFF);
        titleText->setAlpha(0.6f);
        descText->setFormat(Paths::font("vcr.ttf"), 16, 0xFFFFFFFF);
        descText->setAlpha(0.4f);
    }
}

OptionsState::OptionsState() : selectedIndex(0) {
    menuItems = {
        {"Gameplay", "Modify gameplay options and difficulty settings"},
        {"UI Skin", "Change the appearance of notes and interface elements"},
        {"Controls", "Configure your keyboard and controller bindings"},
        {"Exit", "Return to main menu"}
    };
}

OptionsState::~OptionsState() {
    destroy();
}

void OptionsState::create() {
    SwagState::create();
    Engine* engine = Engine::getInstance();
    
    float startY = engine->getWindowHeight() * 0.2f;
    float spacing = engine->getWindowHeight() * 0.15f;
    
    for (size_t i = 0; i < menuItems.size(); i++) {
        float y = startY + (i * spacing);
        optionBoxes.emplace_back(menuItems[i][0], menuItems[i][1], y, i);
        
        engine->addText(optionBoxes[i].titleText);
        engine->addText(optionBoxes[i].descText);

        float centerX = (engine->getWindowWidth() - optionBoxes[i].titleText->getWidth()) / 2;
        optionBoxes[i].titleText->setPosition(centerX, y);
        centerX = (engine->getWindowWidth() - optionBoxes[i].descText->getWidth()) / 2;
        optionBoxes[i].descText->setPosition(centerX, y + 45);
    }

    changeSelection(0);
    Discord::GetInstance().SetDetails("In the Options Menu!");
    Discord::GetInstance().Update();
}

void OptionsState::update(float deltaTime) {
    SwagState::update(deltaTime);
    Input::UpdateKeyStates();
    Input::UpdateControllerStates();

    handleInput();
    updateOptionBoxes(deltaTime);
}

void OptionsState::render() {
    SDL_Renderer* renderer = SDLManager::getInstance().getRenderer();
    Engine* engine = Engine::getInstance();
    
    SDL_Color topColor = {20, 20, 30, 255};
    SDL_Color bottomColor = {40, 40, 60, 255};
    drawGradientRect(renderer, 0, 0, engine->getWindowWidth(), engine->getWindowHeight(), topColor, bottomColor, true);

    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(optionBoxes.size())) {
        const auto& selectedBox = optionBoxes[selectedIndex];
        float highlightY = selectedBox.y - 10;
        float highlightHeight = 80;
        
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

    for (size_t i = 0; i < optionBoxes.size(); i++) {
        const auto& box = optionBoxes[i];
        if (box.titleText && box.descText) {
            if (i != static_cast<size_t>(selectedIndex)) {
                float wave = std::sin(time * 2.0f + i * 0.5f) * 3.0f;
                box.titleText->setPosition(box.titleText->getX(), box.y + wave);
                box.descText->setPosition(box.descText->getX(), box.y + 45 + wave);
            }
            
            box.titleText->render();
            box.descText->render();
        }
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 30);    
    SDL_RenderDrawLine(renderer, 0, 40, engine->getWindowWidth(), 40);    
    SDL_RenderDrawLine(renderer, 0, engine->getWindowHeight() - 40, engine->getWindowWidth(), engine->getWindowHeight() - 40);
}

void OptionsState::handleInput() {
    if (Input::justPressed(SDL_SCANCODE_ESCAPE) || Input::justPressed(SDL_SCANCODE_BACKSPACE)) {
        Engine::getInstance()->switchState(new MainMenuState());
        return;
    }

    if (Input::justPressed(SDL_SCANCODE_UP)) {
        changeSelection(-1);
    }

    if (Input::justPressed(SDL_SCANCODE_DOWN)) {
        changeSelection(1);
    }

    if (Input::justPressed(SDL_SCANCODE_RETURN)) {
        const std::string& selectedOption = menuItems[selectedIndex][0];
        
        if (selectedOption == "Gameplay") {
            openGameplayOptions();
        }
        else if (selectedOption == "UI Skin") {
            openSkinOptions();
        }
        else if (selectedOption == "Controls") {
            openControlsOptions();
        }
        else if (selectedOption == "Exit") {
            startTransitionOut(0.5f);
            Engine::getInstance()->switchState(new MainMenuState());
        }
    }
}

void OptionsState::changeSelection(int change) {
    selectedIndex = (selectedIndex + change) % static_cast<int>(menuItems.size());
    if (selectedIndex < 0) selectedIndex = menuItems.size() - 1;

    for (size_t i = 0; i < optionBoxes.size(); i++) {
        optionBoxes[i].setSelected(i == static_cast<size_t>(selectedIndex));
    }
}

void OptionsState::updateOptionBoxes(float deltaTime) {
    static float time = 0.0f;
    time += deltaTime * 2.0f;

    Engine* engine = Engine::getInstance();
    for (auto& box : optionBoxes) {
        float centerX = (engine->getWindowWidth() - box.titleText->getWidth()) / 2;
        float centerDescX = (engine->getWindowWidth() - box.descText->getWidth()) / 2;
        
        if (&box != &optionBoxes[selectedIndex]) {
            float wave = std::sin(time + box.id * 0.5f) * 3.0f;
            box.titleText->setPosition(centerX, box.y + wave);
            box.descText->setPosition(centerDescX, box.y + 45 + wave);
        } else {
            box.titleText->setPosition(centerX, box.y);
            box.descText->setPosition(centerDescX, box.y + 45);
        }
    }
}

void OptionsState::openGameplayOptions() {
    startTransitionOut(0.5f);
    Engine::getInstance()->switchState(new GameplayOptionsState());
}

void OptionsState::openSkinOptions() {
    startTransitionOut(0.5f);
    Engine::getInstance()->switchState(new SkinOptionsState());
}

void OptionsState::openControlsOptions() {
    startTransitionOut(0.5f);
    Engine::getInstance()->switchState(new ControlsOptionsState());
}

void OptionsState::destroy() {
    Engine* engine = Engine::getInstance();

    optionBoxes.clear();
}
