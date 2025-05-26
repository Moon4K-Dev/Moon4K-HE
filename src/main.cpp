#ifdef __MINGW32__
#include "../engine/core/Engine.h"
#include "gameName/states/PlayState.h"
#include "../engine/input/Input.h"
#include "../moon4k/states/SplashState.h"
#elif defined(__SWITCH__)
#include "../engine/core/Engine.h"
#include "gameName/states/PlayState.h"
#include "../engine/input/Input.h"
#include "../moon4k/states/SplashState.h"
#include <switch.h>
#else
#include "engine/core/Engine.h"
#include "engine/input/Input.h"
#include "moon4k/states/PlayState.h"
#include "engine/utils/Discord.h"
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
    #endif
    
    int width = 1280;
    int height = 720;
    int fps = 60;
    bool debug = true;
    Engine engine(width, height, "Moon4K - Hamburger Engine", fps);
    engine.debugMode = debug;
    
    PlayState* splashState = new PlayState();
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
    #endif
    
    #ifdef __MINGW32__
    // nun
    #elif defined(__SWITCH__)
    // nun
    #else
    Discord::GetInstance().Shutdown();
    #endif
    return 0;
}
