#include "GameConfig.h"
#include "../../engine/utils/Log.h"
#include <fstream>
#include <filesystem>

GameConfig* GameConfig::instance = nullptr;

GameConfig::GameConfig() {
    loadConfig();
}

GameConfig* GameConfig::getInstance() {
    if (!instance) {
        instance = new GameConfig();
    }
    return instance;
}

void GameConfig::loadConfig() {
    std::ifstream file("assets/config.json");
    if (!file.is_open()) {
        config = {
            {"mainBinds", {
                {"left", "A"},
                {"down", "S"},
                {"up", "K"},
                {"right", "L"}
            }},
            {"altBinds", {
                {"left", "ArrowLeft"},
                {"down", "ArrowDown"},
                {"up", "ArrowUp"},
                {"right", "ArrowRight"}
            }},
            {"nxBinds", {
                {"left", "X"},
                {"down", "A"},
                {"up", "Y"},
                {"right", "B"}
            }},
            {"nxAltBinds", {
                {"left", "DPAD_LEFT"},
                {"down", "DPAD_DOWN"},
                {"up", "DPAD_UP"},
                {"right", "DPAD_RIGHT"}
            }},
            {"gameConfig", {
                {"downscroll", false},
                {"ghostTapping", true},
                {"noteskin", "default"},
                {"scrollSpeed", 1.0},
                {"songOffset", 0.0}
            }}
        };
        saveConfig();        
        downscroll = false;
        ghostTapping = true;
        noteskin = "default";
        scrollSpeed = 1.0f;
        songOffset = 0.0f;
        Log::getInstance().info("Created default config.json");
        return;
    }

    try {
        file >> config;
        
        if (config.contains("gameConfig")) {
            auto& gameConfig = config["gameConfig"];
            downscroll = gameConfig.value("downscroll", false);
            ghostTapping = gameConfig.value("ghostTapping", false);
            noteskin = gameConfig.value("noteskin", "default");
            scrollSpeed = gameConfig.value("scrollSpeed", 1.0f);
            songOffset = gameConfig.value("songOffset", 0.0f);
        } else {
            downscroll = false;
            ghostTapping = false;
            noteskin = "default";
            scrollSpeed = 1.0f;
            songOffset = 0.0f;
        }
    } catch (const std::exception& e) {
        Log::getInstance().error("Error parsing config.json: " + std::string(e.what()));
    }
}

void GameConfig::setDownscroll(bool value) {
    downscroll = value;
    config["gameConfig"]["downscroll"] = value;
    saveConfig();
}

void GameConfig::setGhostTapping(bool value) {
    ghostTapping = value;
    config["gameConfig"]["ghostTapping"] = value;
    saveConfig();
}

void GameConfig::setNoteskin(const std::string& value) {
    noteskin = value;
    config["gameConfig"]["noteskin"] = value;
    saveConfig();
}

void GameConfig::setScrollSpeed(float value) {
    scrollSpeed = value;
    config["gameConfig"]["scrollSpeed"] = value;
    saveConfig();
}

void GameConfig::setSongOffset(float value) {
    songOffset = value;
    config["gameConfig"]["songOffset"] = value;
    saveConfig();
}

void GameConfig::saveConfig() {
    std::ofstream file("assets/config.json");
    if (!file.is_open()) {
        Log::getInstance().error("Could not open config.json for writing");
        return;
    }

    try {
        file << config.dump(4);
    } catch (const std::exception& e) {
        Log::getInstance().error("Error saving config.json: " + std::string(e.what()));
    }
} 