#ifdef __MINGW32__
#include "../engine/core/Engine.h"
#include "../moon4k/states/SplashState.h"
#include "../engine/input/Input.h"
#elif defined(__SWITCH__)
#include "../engine/core/Engine.h"
#include "../moon4k/states/SplashState.h"
#include "../engine/input/Input.h"
#include <switch.h>
#else
#include "engine/core/Engine.h"
#include "engine/input/Input.h"
#include "moon4k/states/SplashState.h"
#include "engine/utils/Discord.h"
#include <steam/steam_api.h>
#endif

int main(int argc, char** argv) {
    #ifdef __MINGW32__
    // nun
    #elif defined(__SWITCH__)
    // nun
    #else
    Discord::GetInstance().Initialize("1139936005785407578");    
    Discord::GetInstance().SetState("Moon4K - Hamburger Engine");
    Discord::GetInstance().SetDetails("Working on the Hamburger Engine port!");
    Discord::GetInstance().SetLargeImage("rpcicon");
    Discord::GetInstance().SetLargeImageText(":3");
    Discord::GetInstance().SetSmallImage("");
    Discord::GetInstance().SetSmallImageText("");    
    Discord::GetInstance().Update();

    if (!SteamAPI_Init()) {
        std::cerr << "Failed to initialize Steamworks!" << std::endl;
        return 1;
    }

    std::cout << "Steamworks initialized!" << std::endl;

    const char* username = SteamFriends()->GetPersonaName();
    std::cout << "Logged in as: " << username << std::endl;
    #endif
    
    int width = 1280;
    int height = 720;
    int fps = 60;
    bool debug = true;
    Engine engine(width, height, "Moon4K - Hamburger Engine", fps);
    engine.debugMode = debug;
    
    SplashState* splashState = new SplashState();
    engine.pushState(splashState);
    
    #ifdef __SWITCH__
    while (appletMainLoop()) {
        Input::UpdateControllerStates();
        if (Input::isControllerButtonPressed(SDL_CONTROLLER_BUTTON_BACK) || 
            Input::isControllerButtonPressed(SDL_CONTROLLER_BUTTON_START)) {
            engine.quit();
            break;
        }
        engine.run();
    }
    #else
    engine.run();
    SteamAPI_RunCallbacks();
    #endif
    
    #ifdef __MINGW32__
    // nun
    #elif defined(__SWITCH__)
    // nun
    #else
    Discord::GetInstance().Shutdown();
    SteamAPI_Shutdown();
    #endif
    return 0;
}
