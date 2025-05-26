#pragma once

#include <string>
#include "../backend/json.hpp"

class GameConfig {
private:
    static GameConfig* instance;
    nlohmann::json config;
    float scrollSpeed = 1.0f;
    float songOffset = 0.0f;
    bool downscroll = false;
    bool ghostTapping = false;
    std::string noteskin = "default";

    GameConfig();
    void loadConfig();

public:
    static GameConfig* getInstance();
    
    float getScrollSpeed() const { return scrollSpeed; }
    void setScrollSpeed(float speed) { scrollSpeed = speed; }

    float getSongOffset() const { return songOffset; }
    void setSongOffset(float offset) { songOffset = offset; }

    bool isDownscroll() const { return downscroll; }
    bool isGhostTapping() const { return ghostTapping; }
    std::string getNoteskin() const { return noteskin; }
    
    void setDownscroll(bool value);
    void setGhostTapping(bool value);
    void setNoteskin(const std::string& value);
    void saveConfig();
}; 