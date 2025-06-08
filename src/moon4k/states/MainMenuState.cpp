#include "MainMenuState.h"
#include "../../engine/core/Engine.h"
#include "../../engine/input/Input.h"
#include "../../engine/core/SDLManager.h"
#include "../../engine/utils/Paths.h"
#include "FreeplayState.h"
#include "OnlineDLState.h"
#include "OptionsState.h"
#include "ProfileSubState.h"

MainMenuState::MainMenuState() 
    : titleSprite(nullptr)
    , currentSelection(0) {
    menuTexts = {"Solo", "Browse Online Levels", "Settings", "Exit"};
}

MainMenuState::~MainMenuState() {
    destroy();
}

void MainMenuState::create() {
    SwagState::create();

    Engine* engine = Engine::getInstance();
    
    SDL_SetRenderDrawColor(SDLManager::getInstance().getRenderer(), 0, 0, 0, 255);

    titleSprite = new Sprite(Paths::image("sexylogobyhiro"));    
    titleSprite->setScale(0.25f, 0.25f);
    titleSprite->setPosition(
        (engine->getWindowWidth() - titleSprite->getWidth() * 0.25f) / 2,
        50
    );
    engine->addSprite(titleSprite);

    float startY = engine->getWindowHeight() * 0.5f;
    for (size_t i = 0; i < menuTexts.size(); i++) {
        Text* txt = new Text();
        txt->setFormat(Paths::font("vcr.ttf"), 32, 0xFFFFFFFF);
        txt->setText(menuTexts[i]);
        txt->setPosition(
            (engine->getWindowWidth() - txt->getWidth()) / 2,
            startY + (i * 60)
        );
        menuItems.push_back(txt);
        engine->addText(txt);
    }

    changeSelection();

    char buffer[256];
    snprintf(buffer, sizeof(buffer), "In the Main Menu!");
    Discord::GetInstance().SetDetails(buffer);
    Discord::GetInstance().Update();

    if (SteamAPI_IsSteamRunning() && SteamUser() && SteamFriends()) {
        const char* userName = SteamFriends()->GetPersonaName();

        userNameText = new Text();
        userNameText->setFormat(Paths::font("vcr.ttf"), 24, 0xFFFFFFFF);
        std::string displayName = "Logged in as ";
        displayName += userName;
        userNameText->setText(displayName);

        CSteamID steamID = SteamUser()->GetSteamID();
        int avatarInt = SteamFriends()->GetMediumFriendAvatar(steamID);
        if (avatarInt != 0) {
            uint32 width, height;
            if (SteamUtils()->GetImageSize(avatarInt, &width, &height)) {
                uint8* avatarRGBA = new uint8[4 * width * height];
                if (SteamUtils()->GetImageRGBA(avatarInt, avatarRGBA, 4 * width * height)) {
                    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
                        avatarRGBA, width, height, 32, 4 * width,
                        0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000
                    );

                    if (surface) {
                        SDL_Texture* avatarTexture = SDL_CreateTextureFromSurface(SDLManager::getInstance().getRenderer(), surface);
                        SDL_FreeSurface(surface);
                        delete[] avatarRGBA;

                        if (avatarTexture) {
                            avatarBorderSprite = new Sprite();
                            SDL_Renderer* renderer = SDLManager::getInstance().getRenderer();
                            SDL_Surface* borderSurface = SDL_CreateRGBSurface(0, width + 6, height + 6, 32, 0, 0, 0, 0);
                            SDL_FillRect(borderSurface, nullptr, SDL_MapRGB(borderSurface->format, 255, 0, 255));
                            SDL_Texture* borderTexture = SDL_CreateTextureFromSurface(renderer, borderSurface);
                            SDL_FreeSurface(borderSurface);
                            avatarBorderSprite->setTexture(borderTexture);
                            avatarBorderSprite->setScale(1.0f, 1.0f);
                            
                            avatarSprite = new Sprite();
                            avatarSprite->setTexture(avatarTexture);
                            avatarSprite->setScale(1.0f, 1.0f);
                            
                            int padding = 20;
                            int borderPadding = 3;
                            
                            avatarBorderSprite->setPosition(
                                padding, 
                                engine->getWindowHeight() - padding - height - borderPadding * 2
                            );
                            
                            avatarSprite->setPosition(
                                padding + borderPadding,
                                engine->getWindowHeight() - padding - height - borderPadding
                            );
                            
                            userNameText->setPosition(
                                padding + width + borderPadding * 2 + 10,
                                engine->getWindowHeight() - padding - userNameText->getHeight()
                            );
                            
                            engine->addSprite(avatarBorderSprite);
                            engine->addSprite(avatarSprite);
                            engine->addText(userNameText);
                        }
                    }
                } else {
                    delete[] avatarRGBA;
                }
            }
        }
    }
}

void MainMenuState::update(float deltaTime) {
    SwagState::update(deltaTime);

    if (!isTransitioning()) {
        Input::UpdateKeyStates();
        Input::UpdateControllerStates();

        if (avatarSprite) {
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
            
            SDL_Rect avatarRect = {
                static_cast<int>(avatarSprite->getX()),
                static_cast<int>(avatarSprite->getY()),
                static_cast<int>(avatarSprite->getWidth()),
                static_cast<int>(avatarSprite->getHeight())
            };
            
            bool isAvatarHovered = (mouseX >= avatarRect.x && mouseX < avatarRect.x + avatarRect.w &&
                                  mouseY >= avatarRect.y && mouseY < avatarRect.y + avatarRect.h);
            
            if (isAvatarHovered && Input::justPressed(SDL_SCANCODE_RETURN)) {
                Log::getInstance().info("Opening Profile SubState");
                openSubState(new ProfileSubState());
                return;
            }
        }

        if (Input::justPressed(SDL_SCANCODE_UP)) {
            changeSelection(-1);
        }
        if (Input::justPressed(SDL_SCANCODE_DOWN)) {
            changeSelection(1);
        }

        if (Input::justPressed(SDL_SCANCODE_RETURN)) {
            switch (currentSelection) {
                case 0: // single player
                    startTransitionOut(0.5f);
                    Engine::getInstance()->switchState(new FreeplayState());
                    break;
                case 1: // online level download shit
                    startTransitionOut(0.5f);
                    Engine::getInstance()->switchState(new OnlineDLState());
                    break;
                case 2: // settigns
                    startTransitionOut(0.5f);
                    Engine::getInstance()->switchState(new OptionsState());
                    break;
                case 3: // quitting the game
                    Engine::getInstance()->quit();
                    break;
            }
        }
    }
}

void MainMenuState::render() {
    SDL_SetRenderDrawColor(SDLManager::getInstance().getRenderer(), 0, 0, 0, 255);
    
    if (titleSprite) {
        titleSprite->render();
    }
    
    for (auto item : menuItems) {
        if (item) {
            item->render();
        }
    }

    if (userNameText) {
        userNameText->render();
    }
    if (avatarBorderSprite) {
        avatarBorderSprite->render();
    }
    if (avatarSprite) {
        avatarSprite->render();
    }

    SwagState::render();
}

void MainMenuState::destroy() {
    titleSprite = nullptr;
    avatarSprite = nullptr;
    avatarBorderSprite = nullptr;
    userNameText = nullptr;
    menuItems.clear();

    SwagState::destroy();
}

void MainMenuState::changeSelection(int change) {
    currentSelection += change;

    if (currentSelection < 0) {
        currentSelection = static_cast<int>(menuTexts.size() - 1);
    }
    if (currentSelection >= static_cast<int>(menuTexts.size())) {
        currentSelection = 0;
    }

    for (size_t i = 0; i < menuItems.size(); i++) {
        if (i == static_cast<size_t>(currentSelection)) {
            menuItems[i]->setFormat(Paths::font("vcr.ttf"), 38, 0xFFFF00FF);
        } else {
            menuItems[i]->setFormat(Paths::font("vcr.ttf"), 32, 0xFFFFFFFF);
        }
        
        Engine* engine = Engine::getInstance();
        menuItems[i]->setPosition(
            (engine->getWindowWidth() - menuItems[i]->getWidth()) / 2,
            engine->getWindowHeight() * 0.5f + (i * 60)
        );
    }
}