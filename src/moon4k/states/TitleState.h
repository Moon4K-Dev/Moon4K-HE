#pragma once

#include "../../engine/core/State.h"
#include "../../engine/graphics/Sprite.h"
#include "../../engine/graphics/Text.h"
#include "../SwagState.h"
#include "../../engine/utils/Log.h"
#include "../../engine/utils/Paths.h"
#include "../../engine/audio/SoundManager.h"
#include "../../engine/core/SDLManager.h"
#include "../../engine/core/Engine.h"
#include "FreeplayState.h"

class TitleState : public SwagState {
public:
    static TitleState* instance;
    
    TitleState();
    ~TitleState() override;
    
    void create() override;
    void update(float deltaTime) override;
    void render() override;
    void destroy() override;

private:
    Sprite* titleSprite;
    Text* songText;
    std::vector<std::string> titleMusic;
    std::string currentMusic;
};