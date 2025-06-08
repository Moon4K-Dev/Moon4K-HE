#include "ProfileSubState.h"
#include "../../engine/core/Engine.h"
#include "../../engine/core/SDLManager.h"
#include "../../engine/input/Input.h"
#include "../../engine/utils/Paths.h"

ProfileSubState::ProfileSubState() 
    : bgOverlay(nullptr)
    , avatarSprite(nullptr)
    , avatarBorderSprite(nullptr)
    , userNameText(nullptr)
    , steamIDText(nullptr)
    , statusText(nullptr)
    , closeButtonSprite(nullptr)
    , isCloseHovered(false) {
}

ProfileSubState::~ProfileSubState() {
    destroy();
}

void ProfileSubState::create() {
    Engine* engine = Engine::getInstance();

    SDL_Renderer* renderer = SDLManager::getInstance().getRenderer();
    SDL_Surface* overlaySurface = SDL_CreateRGBSurface(0, engine->getWindowWidth(), engine->getWindowHeight(), 32, 0, 0, 0, 0);
    SDL_FillRect(overlaySurface, nullptr, SDL_MapRGBA(overlaySurface->format, 0, 0, 0, 180));
    SDL_Texture* overlayTexture = SDL_CreateTextureFromSurface(renderer, overlaySurface);
    SDL_FreeSurface(overlaySurface);
    
    bgOverlay = new Sprite();
    bgOverlay->setTexture(overlayTexture);
    engine->addSprite(bgOverlay);

    loadSteamProfile();
    loadAchievements();
    loadStats();
    createCloseButton();
}

void ProfileSubState::loadSteamProfile() {
    Engine* engine = Engine::getInstance();
    
    if (!SteamAPI_IsSteamRunning() || !SteamUser() || !SteamFriends()) {
        return;
    }

    const char* userName = SteamFriends()->GetPersonaName();
    CSteamID steamID = SteamUser()->GetSteamID();
    
    userNameText = new Text();
    userNameText->setFormat(Paths::font("vcr.ttf"), 32, 0xFFFFFFFF);
    userNameText->setText(userName);
    userNameText->setPosition(
        (engine->getWindowWidth() - userNameText->getWidth()) / 2,
        100
    );
    engine->addText(userNameText);

    steamIDText = new Text();
    steamIDText->setFormat(Paths::font("vcr.ttf"), 24, 0xAAAAAAAA);
    steamIDText->setText("Steam ID: " + std::to_string(steamID.ConvertToUint64()));
    steamIDText->setPosition(
        (engine->getWindowWidth() - steamIDText->getWidth()) / 2,
        150
    );
    engine->addText(steamIDText);

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
                        SDL_Surface* borderSurface = SDL_CreateRGBSurface(0, width + 6, height + 6, 32, 0, 0, 0, 0);
                        SDL_FillRect(borderSurface, nullptr, SDL_MapRGB(borderSurface->format, 255, 0, 255));
                        SDL_Texture* borderTexture = SDL_CreateTextureFromSurface(SDLManager::getInstance().getRenderer(), borderSurface);
                        SDL_FreeSurface(borderSurface);
                        avatarBorderSprite->setTexture(borderTexture);
                        
                        avatarSprite = new Sprite();
                        avatarSprite->setTexture(avatarTexture);
                        
                        int centerX = (engine->getWindowWidth() - width) / 2;
                        avatarBorderSprite->setPosition(centerX - 3, 200);
                        avatarSprite->setPosition(centerX, 203);
                        
                        engine->addSprite(avatarBorderSprite);
                        engine->addSprite(avatarSprite);
                    }
                }
            }
        }
    }

    EPersonaState personaState = SteamFriends()->GetFriendPersonaState(steamID);
    std::string status;
    switch (personaState) {
        case k_EPersonaStateOffline: status = "Offline"; break;
        case k_EPersonaStateOnline: status = "Online"; break;
        case k_EPersonaStateBusy: status = "Busy"; break;
        case k_EPersonaStateAway: status = "Away"; break;
        case k_EPersonaStateSnooze: status = "Snooze"; break;
        case k_EPersonaStateLookingToTrade: status = "Looking to Trade"; break;
        case k_EPersonaStateLookingToPlay: status = "Looking to Play"; break;
        default: status = "Unknown"; break;
    }
    
    statusText = new Text();
    statusText->setFormat(Paths::font("vcr.ttf"), 24, 0xFFFFFFFF);
    statusText->setText("Status: " + status);
    statusText->setPosition(
        (engine->getWindowWidth() - statusText->getWidth()) / 2,
        300
    );
    engine->addText(statusText);
}

void ProfileSubState::loadAchievements() {
    if (!SteamAPI_IsSteamRunning() || !SteamUserStats()) {
        return;
    }

    Engine* engine = Engine::getInstance();
    int achievementCount = SteamUserStats()->GetNumAchievements();
    float startY = 350;
    
    Text* achievementTitle = new Text();
    achievementTitle->setFormat(Paths::font("vcr.ttf"), 28, 0xFFFFFFFF);
    achievementTitle->setText("Achievements:");
    achievementTitle->setPosition(
        (engine->getWindowWidth() - achievementTitle->getWidth()) / 2,
        startY
    );
    achievementTexts.push_back(achievementTitle);
    engine->addText(achievementTitle);

    for (int i = 0; i < achievementCount && i < 5; i++) {
        const char* achievementName = SteamUserStats()->GetAchievementName(i);
        const char* achievementDesc = SteamUserStats()->GetAchievementDisplayAttribute(achievementName, "desc");
        bool achieved = false;
        SteamUserStats()->GetAchievement(achievementName, &achieved);

        Text* achievementText = new Text();
        achievementText->setFormat(Paths::font("vcr.ttf"), 20, achieved ? 0x00FF00FF : 0xFF0000FF);
        achievementText->setText(std::string(achievementDesc) + (achieved ? " (Unlocked)" : " (Locked)"));
        achievementText->setPosition(
            (engine->getWindowWidth() - achievementText->getWidth()) / 2,
            startY + 40 + (i * 30)
        );
        achievementTexts.push_back(achievementText);
        engine->addText(achievementText);
    }
}

void ProfileSubState::loadStats() {
    if (!SteamAPI_IsSteamRunning() || !SteamUserStats()) {
        return;
    }

    Engine* engine = Engine::getInstance();
    float startY = 550;

    Text* statsTitle = new Text();
    statsTitle->setFormat(Paths::font("vcr.ttf"), 28, 0xFFFFFFFF);
    statsTitle->setText("Stats:");
    statsTitle->setPosition(
        (engine->getWindowWidth() - statsTitle->getWidth()) / 2,
        startY
    );
    statTexts.push_back(statsTitle);
    engine->addText(statsTitle);

    int32 totalPlayTime = 0;
    SteamUserStats()->GetStat("TotalPlayTime", &totalPlayTime);
    
    Text* playTimeText = new Text();
    playTimeText->setFormat(Paths::font("vcr.ttf"), 20, 0xFFFFFFFF);
    playTimeText->setText("Total Play Time: " + std::to_string(totalPlayTime) + " minutes");
    playTimeText->setPosition(
        (engine->getWindowWidth() - playTimeText->getWidth()) / 2,
        startY + 40
    );
    statTexts.push_back(playTimeText);
    engine->addText(playTimeText);
}

void ProfileSubState::createCloseButton() {
    Engine* engine = Engine::getInstance();
    
    SDL_Renderer* renderer = SDLManager::getInstance().getRenderer();
    SDL_Surface* buttonSurface = SDL_CreateRGBSurface(0, 30, 30, 32, 0, 0, 0, 0);
    SDL_FillRect(buttonSurface, nullptr, SDL_MapRGB(buttonSurface->format, 255, 0, 255));
    SDL_Texture* buttonTexture = SDL_CreateTextureFromSurface(renderer, buttonSurface);
    SDL_FreeSurface(buttonSurface);
    
    closeButtonSprite = new Sprite();
    closeButtonSprite->setTexture(buttonTexture);
    closeButtonSprite->setPosition(
        engine->getWindowWidth() - 50,
        20
    );
    engine->addSprite(closeButtonSprite);
}

void ProfileSubState::update(float deltaTime) {
    Engine* engine = Engine::getInstance();
    
    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);
    
    SDL_Rect closeButtonRect = {
        static_cast<int>(engine->getWindowWidth() - 50),
        20,
        30,
        30
    };
    
    isCloseHovered = (mouseX >= closeButtonRect.x && mouseX < closeButtonRect.x + closeButtonRect.w &&
                      mouseY >= closeButtonRect.y && mouseY < closeButtonRect.y + closeButtonRect.h);
    
    if (isCloseHovered && Input::justPressed(SDL_SCANCODE_RETURN) || Input::justPressed(SDL_SCANCODE_ESCAPE)) {
        if (getParentState()) {
            getParentState()->closeSubState();
        }
    }
}

void ProfileSubState::render() {
    if (bgOverlay) bgOverlay->render();
    if (avatarBorderSprite) avatarBorderSprite->render();
    if (avatarSprite) avatarSprite->render();
    if (userNameText) userNameText->render();
    if (steamIDText) steamIDText->render();
    if (statusText) statusText->render();
    if (closeButtonSprite) closeButtonSprite->render();
    
    for (auto text : achievementTexts) {
        if (text) text->render();
    }
    
    for (auto text : statTexts) {
        if (text) text->render();
    }
}

void ProfileSubState::destroy() {
    Engine* engine = Engine::getInstance();
    
    if (bgOverlay) {
        delete bgOverlay;
        bgOverlay = nullptr;
    }
    
    if (avatarSprite) {
        delete avatarSprite;
        avatarSprite = nullptr;
    }
    
    if (avatarBorderSprite) {
        delete avatarBorderSprite;
        avatarBorderSprite = nullptr;
    }
    
    if (userNameText) {
        delete userNameText;
        userNameText = nullptr;
    }
    
    if (steamIDText) {
        delete steamIDText;
        steamIDText = nullptr;
    }
    
    if (statusText) {
        delete statusText;
        statusText = nullptr;
    }
    
    if (closeButtonSprite) {
        delete closeButtonSprite;
        closeButtonSprite = nullptr;
    }
    
    for (auto text : achievementTexts) {
        if (text) delete text;
    }
    achievementTexts.clear();
    
    for (auto text : statTexts) {
        if (text) delete text;
    }
    statTexts.clear();
} 