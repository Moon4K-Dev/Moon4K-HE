#pragma once

#include "../../engine/core/State.h"
#include "../../engine/graphics/Sprite.h"
#include "../../engine/graphics/Text.h"
#include "../SwagState.h"
#include "../../engine/utils/Log.h"
#include "../../engine/utils/Paths.h"
#include "../../engine/core/Engine.h"
#include "../../engine/input/Input.h"
#include "../../engine/core/SDLManager.h"
#include "../../engine/audio/SoundManager.h"
#include <vector>
#include <string>
#include <filesystem>

class FreeplayState : public SwagState {
public:
    static FreeplayState* instance;
    
    FreeplayState();
    ~FreeplayState() override;
    
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
    
    struct SongNote {
        float strumTime;
        int direction;
        float sustainLength;
    };

    struct SongSection {
        std::vector<SongNote> sectionNotes;
        bool mustHitSection;
        float bpm;
    };
    
    struct SongData {
        std::string song;
        std::vector<SongSection> notes;
        float bpm;
        int sections;
        float speed;
        int keyCount;
        std::vector<float> sectionLengths;
        bool validScore;
    };
    
    SongData songData;
};
