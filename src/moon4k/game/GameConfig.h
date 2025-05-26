#pragma once

#include <string>
#include "../backend/json.hpp"

class GameConfig {
private:
    static GameConfig* instance;
    nlohmann::json config;
    bool downscroll = false;
    bool ghostTapping = false;
    std::string noteskin = "default";
    float scrollSpeed = 1.0f;
    float songOffset = 0.0f;

    GameConfig();
    void loadConfig();
    void saveConfig();

public:
    static GameConfig* getInstance();

    bool getDownscroll() const { return downscroll; }
    bool getGhostTapping() const { return ghostTapping; }
    const std::string& getNoteskin() const { return noteskin; }
    float getScrollSpeed() const { return scrollSpeed; }
    float getSongOffset() const { return songOffset; }

    void setDownscroll(bool value);
    void setGhostTapping(bool value);
    void setNoteskin(const std::string& value);
    void setScrollSpeed(float value);
    void setSongOffset(float value);
}; 