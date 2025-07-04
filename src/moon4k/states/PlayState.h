#pragma once

#include "../SwagState.h"
#include "../../engine/graphics/Sprite.h"
#include "../../engine/graphics/AnimatedSprite.h"
#include "../../engine/input/Input.h"
#include "../../engine/audio/Sound.h"
#include "../substates/PauseSubState.h"
#include "../../engine/utils/Log.h"
#include "../../engine/utils/Paths.h"
#include "../../engine/graphics/VideoPlayer.h"
#include "../game/Song.h"
#include "../game/Note.h"
#include "../game/GameConfig.h"
#include "../../engine/core/SDLManager.h"
#include "../../engine/graphics/Text.h"
#include "../../engine/core/Engine.h"
#include "../ui/StrumNote.h"
#include "../ui/UI.h"
#include <vector>
#include <array>
#include <string>
#include <map>

void playStateKeyboardCallback(unsigned char key, int x, int y);

struct KeyBinding {
    SDL_Scancode primary;
    SDL_Scancode alternate;
};

struct NXBinding {
    SDL_GameControllerButton primary;
    SDL_GameControllerButton alternate;
};

class PlayState : public SwagState {
public:
    PlayState(const std::string& songName, const std::string& difficulty = "");
    ~PlayState();

    void create() override;
    void update(float deltaTime) override;
    void render() override;
    void destroy() override;

    void openSubState(SubState* subState);
    void generateSong(std::string dataPath);
    void startSong();
    void startCountdown();
    void generateNotes();
    void goodNoteHit(Note* note);
    void noteMiss(int direction);
    void sortByShit(Note* a, Note* b);
    void endSong();

    static PlayState* instance;
    static SwagSong SONG;
    static Sound* inst;
    static VideoPlayer* videoPlayer;
    static Sound* voices;
    bool startingSong = false;
    bool startedCountdown = false;

    std::vector<Note*> unspawnNotes;
    std::vector<Note*> spawnNotes;
    int combo = 0;
    int score = 0;
    int misses = 0;
    float accuracy = 0.00f;
    float totalNotesHit = 0.0f;
    int totalPlayed = 0;
    bool pfc = false;
    std::string curRank = "P";

    float health = 1.0f;
    const float HEALTH_GAIN = 0.1f;
    const float HEALTH_LOSS = 0.04f;
    const float HEALTH_DRAIN = 0.005f;
    bool isDead = false;

    bool loadVideo(const std::string& path);
    void playVideo();
    void pauseVideo();
    void stopVideo();
    void setVolume(int volume);

private:
    std::string directSongName;
    std::string selectedDifficulty;
    int sections = 0;
    std::vector<SwagSection> sectionLengths;
    int keyCount = 4;
    std::vector<int> timescale;
    
    std::unique_ptr<Sprite> defaultBG;
    std::unique_ptr<Sprite> songBG;
    
    bool isLoading = true;
    bool isCountingDown = false;
    float countdownTime = 3.0f;
    Text* countdownText;
    Text* loadingText;
    
    std::string curSong;
    Sound* vocals = nullptr;
    std::vector<StrumNote*> strumLineNotes;
    std::vector<Note*> notes;
    const float STRUM_X = 42.0f;
    const float STRUM_X_MIDDLESCROLL = -278.0f;
    const int laneOffset = 100;
    
    std::array<KeyBinding, 4> arrowKeys;
    std::array<NXBinding, 4> nxArrowKeys;

    void loadKeybinds();
    void loadSongConfig();
    void handleOpponentNoteHit(float deltaTime);
    void checkAndSetBackground();
    SDL_Scancode getScancodeFromString(const std::string& keyName);
    SDL_GameControllerButton getButtonFromString(const std::string& buttonName);
    void updateAccuracy();
    void updateRank();

    bool isKeyPressed(int keyIndex) const {
        const auto& binding = arrowKeys[keyIndex];
        return Input::pressed(binding.primary) || Input::pressed(binding.alternate);
    }

    bool isKeyJustPressed(int keyIndex) const {
        const auto& binding = arrowKeys[keyIndex];
        return Input::justPressed(binding.primary) || Input::justPressed(binding.alternate);
    }

    bool isKeyJustReleased(int keyIndex) const {
        const auto& binding = arrowKeys[keyIndex];
        return Input::justReleased(binding.primary) || Input::justReleased(binding.alternate);
    }

    bool isNXButtonPressed(int keyIndex) const {
        const auto& binding = nxArrowKeys[keyIndex];
        return Input::isControllerButtonPressed(binding.primary) || Input::isControllerButtonPressed(binding.alternate);
    }

    bool isNXButtonJustPressed(int keyIndex) const {
        const auto& binding = nxArrowKeys[keyIndex];
        return Input::isControllerButtonJustPressed(binding.primary) || Input::isControllerButtonJustPressed(binding.alternate);
    }

    bool isNXButtonJustReleased(int keyIndex) const {
        const auto& binding = nxArrowKeys[keyIndex];
        return Input::isControllerButtonJustReleased(binding.primary) || Input::isControllerButtonJustReleased(binding.alternate);
    }

    void handleInput();
    void updateArrowAnimations();
    std::unique_ptr<UI> ui;
    float pauseCooldown = 0.0f;

    void updateHealth(float deltaTime);
    void gainHealth();
    void loseHealth();

    std::string currentVideoPath;
};