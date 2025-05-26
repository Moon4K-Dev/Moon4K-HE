#pragma once

#include "../../engine/graphics/AnimatedSprite.h"
#include "../game/GameConfig.h"
#include <string>
#include <vector>
#include "../backend/json.hpp"

class StrumNote : public AnimatedSprite {
public:
    StrumNote(float x, float y, int direction = 0, const std::string& noteskin = "default", int keyCount = 4);
    ~StrumNote();

    void update(float deltaTime) override;
    void playAnim(const std::string& anim, bool force = false, bool reversed = false, int frame = 0);
    void loadNoteSkin(const std::string& noteskin = "default", int direction = -1);

private:
    nlohmann::json json;
    std::string noteskin = "default";
    int direction = 0;
    std::vector<float> offsets = {0, 0};

    void centerOffsets();
    void centerOrigin();
};
