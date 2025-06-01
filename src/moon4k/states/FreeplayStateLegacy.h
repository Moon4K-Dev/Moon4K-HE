#pragma once

#include "../SwagState.h"
#include "../../engine/graphics/Sprite.h"
#include "../../engine/graphics/Text.h"
#include "../../engine/utils/Log.h"
#include "../../engine/utils/Paths.h"
#include "../../engine/core/Engine.h"
#include "../../engine/input/Input.h"
#include "../../engine/core/SDLManager.h"
#include "../../engine/audio/SoundManager.h"
#include <vector>
#include <string>
#include <filesystem>

struct SongNote {
    float strumTime;
    int direction;
    float sustainLength;
};

struct SongSection {
    bool mustHitSection;
    float bpm;
    std::vector<SongNote> sectionNotes;
};

struct SongData {
    std::string song;
    float bpm;
    float speed;
    int sections;
    int keyCount;
    bool validScore;
    std::vector<SongSection> notes;
};

class FreeplayStateLegacy : public SwagState {
public:
    static FreeplayStateLegacy* instance;
    
    FreeplayStateLegacy();
    ~FreeplayStateLegacy() override;
    
    void create() override;
    void update(float deltaTime) override;
    void render() override;
    void destroy() override;

private:
    void changeSelection(int change = 0);
    void updateSongList();
    void rescanSongs();
    void updateSongImage();
    void loadSongJson(const std::string& songName);
    void loadSongs();

    std::vector<Text*> songTexts;
    std::vector<std::string> songs;
    int curSelected;
    
    Text* scoreText;
    Text* missText;
    Text* diffText;
    Text* noSongsText;
    
    Sprite* songImage;
    Sprite* menuBG;
    
    std::string selectedSong;
    float songHeight;
    
    SongData songData;
};
