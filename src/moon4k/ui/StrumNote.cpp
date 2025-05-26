#include "StrumNote.h"
#include "../../engine/utils/Log.h"
#include <fstream>
#include <filesystem>

StrumNote::StrumNote(float x, float y, int direction, const std::string& noteskin, int keyCount) 
    : AnimatedSprite(), direction(direction) {
    setPosition(x, y);
    loadNoteSkin(noteskin, direction);
}

StrumNote::~StrumNote() {
}

void StrumNote::update(float deltaTime) {
    AnimatedSprite::update(deltaTime);
}

void StrumNote::playAnim(const std::string& anim, bool force, bool reversed, int frame) {
    playAnimation(anim);
    centerOffsets();
    centerOrigin();

    offsetX = offsetX + offsets[0];
    offsetY = offsetY + offsets[1];
}

void StrumNote::loadNoteSkin(const std::string& noteskin, int direction) {
    std::string configPath = "assets/images/ui-skins/" + noteskin + "/config.json";
    
    if (!std::filesystem::exists(configPath)) {
        Log::getInstance().warning("UI skin config not found: " + configPath);
        this->noteskin = "default";
        configPath = "assets/images/ui-skins/default/config.json";
    } else {
        this->noteskin = noteskin;
    }

    if (direction != -1) {
        this->direction = direction;
    }

    std::ifstream configFile(configPath);
    if (!configFile.is_open()) {
        Log::getInstance().error("Failed to open skin config: " + configPath);
        return;
    }

    try {
        configFile >> json;
        configFile.close();

        if (json.contains("offsets") && json["offsets"].is_array()) {
            offsets.clear();
            for (const auto& offset : json["offsets"]) {
                offsets.push_back(offset.get<float>());
            }
        } else {
            offsets = {0, 0};
        }

        std::string notesPath = "assets/images/ui-skins/" + this->noteskin + "/notes";
        loadFrames(notesPath + ".png", notesPath + ".xml");

        if (json.contains("animations") && json["animations"].is_array() && 
            this->direction < json["animations"].size()) {
            
            auto& anims = json["animations"][this->direction];
            if (anims.size() >= 4) {
                addAnimation("static", anims[0].get<std::string>(), json.value("framerate", 24), false);
                addAnimation("press", anims[1].get<std::string>(), json.value("framerate", 24), false);
                addAnimation("confirm", anims[2].get<std::string>(), json.value("framerate", 24), false);
                addAnimation("note", anims[3].get<std::string>(), json.value("framerate", 24), false);
            }
        }

        float size = json.value("size", 1.0f);
        setScale(size, size);

        playAnimation("static");

    } catch (const std::exception& e) {
        Log::getInstance().error("Failed to parse skin config: " + std::string(e.what()));
    }
}

void StrumNote::centerOffsets() {
    offsetX = (width - width) * 0.5f;
    offsetY = (height - height) * 0.5f;
}

void StrumNote::centerOrigin() {
    offsetX += width * 0.5f;
    offsetY += height * 0.5f;
}
