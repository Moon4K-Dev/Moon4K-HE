#pragma once

#include <string>
#include "../backend/json.hpp"

class GameConfig {
private:
    static GameConfig* instance;
    nlohmann::json config;
    bool downscroll;
    bool ghostTapping;
    std::string noteskin;

    GameConfig();
    void loadConfig();

public:
    static GameConfig* getInstance();
    
    bool isDownscroll() const { return downscroll; }
    bool isGhostTapping() const { return ghostTapping; }
    std::string getNoteskin() const { return noteskin; }
    
    void setDownscroll(bool value);
    void setGhostTapping(bool value);
    void setNoteskin(const std::string& value);
    void saveConfig();
}; 