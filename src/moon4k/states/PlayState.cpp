#include "PlayState.h"
#include <iostream>
#include <algorithm>
#include "../game/Song.h"
#include "../game/Conductor.h"
#include <fstream>
#include <map>
#ifdef __SWITCH__ 
#include "../backend/json.hpp"
#elif defined(__MINGW32__)
#include "../backend/json.hpp"
#else
#include "../backend/json.hpp"
#endif

PlayState* PlayState::instance = nullptr;
SwagSong PlayState::SONG;
Sound* PlayState::inst = nullptr;

PlayState::PlayState() {
    instance = this;
    inst = nullptr;
    vocals = nullptr;
    Note::loadAssets();
    scoreText = new Text();
    scoreText->setFormat("assets/fonts/vcr.ttf", 32, 0xFFFFFFFF);
    
    int windowWidth = Engine::getInstance()->getWindowWidth();
    int windowHeight = Engine::getInstance()->getWindowHeight();
    scoreText->setPosition(windowWidth / 2 - 100, windowHeight - 50);
    updateScoreText();

    loadKeybinds();
}

PlayState::~PlayState() {
    if (vocals != nullptr) {
        delete vocals;
        vocals = nullptr;
    }
    if (inst != nullptr) {
        delete inst;
        inst = nullptr;
    }
    
    for (auto arrow : strumLineNotes) {
        delete arrow;
    }
    strumLineNotes.clear();
    
    for (auto note : notes) {
        delete note;
    }
    notes.clear();
    
    delete scoreText;
    Note::unloadAssets();
    destroy();
}

void PlayState::loadSongConfig() {
    std::ifstream configFile("assets/config.json");
    if (!configFile.is_open()) {
        Log::getInstance().error("Failed to open config.json");
        return;
    }

    try {
        nlohmann::json config;
        configFile >> config;

        if (config.contains("songConfig")) {
            auto songConfig = config["songConfig"];
            std::string songName = songConfig["songName"].get<std::string>();
            std::string difficulty = songConfig["difficulty"].get<std::string>();
            
            std::string fullSongName = difficulty.empty() ? songName : songName + "-" + difficulty;
            generateSong(fullSongName);
        } else {
            Log::getInstance().error("No songConfig found in config.json");
        }
    } catch (const std::exception& ex) {
        Log::getInstance().error("Failed to parse song config: " + std::string(ex.what()));
    }
}

void PlayState::create() {
    Engine* engine = Engine::getInstance();

    loadSongConfig();
    startingSong = true;
    #ifdef __SWITCH__
    // nun
    #elif defined(__MINGW32__)
    // nun
    #else
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "In Game - %s", curSong.c_str());
    Discord::GetInstance().SetState(buffer);
    Discord::GetInstance().Update();
    #endif
}

void PlayState::update(float deltaTime) {
    SwagState::update(deltaTime);

    if (!_subStates.empty()) {
        _subStates.back()->update(deltaTime);
    } 
    else {
        Input::UpdateKeyStates();
        Input::UpdateControllerStates();

        handleInput();
        handleOpponentNoteHit(deltaTime);
        updateArrowAnimations();

        for (auto arrow : strumLineNotes) {
            if (arrow) {
                arrow->update(deltaTime);
            }
        }

        Conductor::songPosition += deltaTime * 1000.0f;

        while (!unspawnNotes.empty()) {
            Note* nextNote = unspawnNotes[0];
            if (nextNote->strumTime - Conductor::songPosition > 1500) {
                break;
            }
            
            notes.push_back(nextNote);
            unspawnNotes.erase(unspawnNotes.begin());
        }

        for (auto note : notes) {
            if (!note) continue;
            
            auto strum = strumLineNotes[note->direction % keyCount];
            if (!strum) continue;

            if (GameConfig::getInstance()->getDownscroll()) {
                note->setY(strum->getY() + (0.45f * (Conductor::songPosition - note->strumTime) * GameConfig::getInstance()->getScrollSpeed()));
            } else {
                note->setY(strum->getY() - (0.45f * (Conductor::songPosition - note->strumTime) * GameConfig::getInstance()->getScrollSpeed()));
            }

            note->update(deltaTime);

            if (note->shouldHit && note->tooLate && !note->wasGoodHit) {
                noteMiss(note->direction);
                note->kill = true;
            }

            if (note->kill || note->strumTime < Conductor::songPosition - 5000) {
                auto it = std::find(notes.begin(), notes.end(), note);
                if (it != notes.end()) {
                    notes.erase(it);
                }
                delete note;
            }
        }

        if (startingSong) {
            if (startedCountdown) {
                Conductor::songPosition += deltaTime * 1000;
                if (Conductor::songPosition >= 0) {
                    startSong();
                }
            }
        }

        if (pauseCooldown > 0) {
            pauseCooldown -= deltaTime;
        }

        if (Input::justPressed(SDL_SCANCODE_RETURN) || Input::isControllerButtonJustPressed(SDL_CONTROLLER_BUTTON_START)) {
            if (_subStates.empty()) {
                if (pauseCooldown <= 0) {
                    PauseSubState* pauseSubState = new PauseSubState();
                    openSubState(pauseSubState);
                    Log::getInstance().info("Pause SubState opened");
                    pauseCooldown = 0.5f;
                }
            } else {
                closeSubState();
                Log::getInstance().info("Pause SubState closed");
            }
        }
    }
}

void PlayState::handleInput() {
    for (size_t i = 0; i < 4; i++) {
        if (i < strumLineNotes.size() && strumLineNotes[i]) {
            bool keyPressed = isKeyPressed(static_cast<int>(i)) || isNXButtonPressed(static_cast<int>(i));
            bool keyJustPressed = isKeyJustPressed(static_cast<int>(i)) || isNXButtonJustPressed(static_cast<int>(i));
            bool keyJustReleased = isKeyJustReleased(static_cast<int>(i)) || isNXButtonJustReleased(static_cast<int>(i));
            
            if (keyJustPressed) {
                strumLineNotes[i]->playAnimation("press");
                
                bool noteHit = false;
                for (auto note : notes) {
                    if (note && note->shouldHit && !note->wasGoodHit && note->direction == static_cast<int>(i) && note->canBeHit) {
                        goodNoteHit(note);
                        noteHit = true;
                        break;
                    }
                }
                
                if (!noteHit && !GameConfig::getInstance()->getGhostTapping()) {
                    noteMiss(static_cast<int>(i));
                }
            }
            else if (keyPressed) {
                for (auto note : notes) {
                    if (note && note->shouldHit && note->isSustainNote && !note->wasGoodHit && 
                        note->direction == static_cast<int>(i) && note->canBeHit) {
                        goodNoteHit(note);
                    }
                }
            }
            else if (keyJustReleased) {
                strumLineNotes[i]->playAnimation("static");
            }
        }
    }
}

void PlayState::updateArrowAnimations() {
    for (size_t i = 4; i < strumLineNotes.size(); i++) {
        auto arrow = strumLineNotes[i];
        if (arrow) {
            int keyIndex = (i - 4) % 4;
            if (arrow->getCurrentAnimation() == "pressed" && !isKeyPressed(keyIndex)) {
                arrow->playAnimation("static");
            }
        }
    }
}

void PlayState::generateSong(std::string dataPath) {
    try {
        std::string songName = dataPath;
        std::string folder = dataPath;
        std::string baseSongName = dataPath;
        
        SONG = Song::loadFromJson(songName, folder);
        if (!SONG.validScore) {
            Log::getInstance().error("Failed to load song data");
            return;
        }
        
        Conductor::changeBPM(SONG.bpm);
        curSong = songName;
        sections = SONG.sections;
        sectionLengths = SONG.sectionLengths;
        keyCount = SONG.keyCount ? SONG.keyCount : 4;
        timescale = SONG.timescale;

        std::cout << "Generated song: " << curSong 
                  << " BPM: " << SONG.bpm 
                  << " Speed: " << SONG.speed 
                  << " Key Count: " << keyCount 
                  << " Sections: " << sections << std::endl;

        if (vocals != nullptr) {
            delete vocals;
            vocals = nullptr;
        }
        if (inst != nullptr) {
            delete inst;
            inst = nullptr;
        }

        std::string instPath = "assets/charts/" + baseSongName + "/" + baseSongName + soundExt;
        inst = new Sound();
        if (!inst->load(instPath)) {
            Log::getInstance().error("Failed to load song audio: " + instPath);
            delete inst;
            inst = nullptr;
        }

        startCountdown();
        generateNotes();
        
    } catch (const std::exception& ex) {
        std::cerr << "Error generating song: " << ex.what() << std::endl;
        Log::getInstance().error("Error generating song: " + std::string(ex.what()));
    }
}

void PlayState::startSong() {
    startingSong = false;
    if (vocals != nullptr) {
        vocals->play();
    }
    if (inst != nullptr) {
        inst->play();
    }
}

void PlayState::startCountdown() {
    startedCountdown = true;
    Conductor::songPosition = 0;
    Conductor::songPosition -= Conductor::crochet * 5;

    int windowWidth = Engine::getInstance()->getWindowWidth();
    int windowHeight = Engine::getInstance()->getWindowHeight();
    
    float yPos = GameConfig::getInstance()->getDownscroll() ? (windowHeight - 150.0f) : 100.0f;
    yPos -= 20;
    
    float totalWidth = laneOffset * (keyCount - 1);
    float startX = (windowWidth - totalWidth) / 2.0f;
    
    for (int i = 0; i < keyCount; i++) {
        std::string noteskin = GameConfig::getInstance()->getNoteskin();
        StrumNote* babyArrow = new StrumNote(0, yPos, i, noteskin, keyCount);
        
        float x = startX + (i * laneOffset);
        babyArrow->setPosition(x, yPos);
        strumLineNotes.push_back(babyArrow);
    }
}

void PlayState::render() {
    for (auto arrow : strumLineNotes) {
        if (arrow && arrow->isVisible()) {
            arrow->render();
        }
    }

    for (auto note : notes) {
        if (note && note->isVisible()) {
            note->render();
        }
    }

    scoreText->render();

    static int lastNoteCount = 0;
    if (notes.size() != lastNoteCount) {
        Log::getInstance().info("Active notes: " + std::to_string(notes.size()));
        lastNoteCount = notes.size();
    }

    if (!_subStates.empty()) {
        _subStates.back()->render();
    }
}

void PlayState::generateNotes() {
    unspawnNotes.clear();
    spawnNotes.clear();
    notes.clear();

    Log::getInstance().info("Generating notes from " + std::to_string(SONG.notes.size()) + " sections");

    int totalNotes = 0;
    for (const auto& section : SONG.notes) {
        Log::getInstance().info("Processing section with " + std::to_string(section.sectionNotes.size()) + " notes");
        
        Conductor::recalculateStuff(1.0f);

        for (const auto& noteData : section.sectionNotes) {
            if (noteData.size() >= 2) {
                float strumTime = noteData[0];
                int direction = static_cast<int>(noteData[1]);
                
                if (direction >= keyCount) {
                    Log::getInstance().info("Skipping note with direction " + std::to_string(direction) + " (exceeds keyCount " + std::to_string(keyCount) + ")");
                    continue;
                }

                float sustainLength = noteData.size() > 3 ? noteData[3] : 0.0f;

                StrumNote* strum = nullptr;
                if (direction < strumLineNotes.size()) {
                    strum = strumLineNotes[direction];
                }
                
                if (!strum) {
                    Log::getInstance().error("No strum note found for direction " + std::to_string(direction));
                    continue;
                }

                float daStrumTime = strumTime + (GameConfig::getInstance()->getSongOffset());
                int daNoteData = direction % keyCount;

                Note* oldNote = nullptr;
                if (!spawnNotes.empty()) {
                    oldNote = spawnNotes.back();
                }

                Note* swagNote = new Note(strum->getX(), strum->getY(), daNoteData, daStrumTime, 
                                        GameConfig::getInstance()->getNoteskin(), false, keyCount);
                swagNote->sustainLength = sustainLength;
                swagNote->lastNote = oldNote;
                swagNote->playAnim("note");

                spawnNotes.push_back(swagNote);
                totalNotes++;

                float susLength = sustainLength / Conductor::stepCrochet;
                
                if (susLength > 0) {
                    Log::getInstance().info("Creating sustain note with length " + std::to_string(susLength));
                }
                
                for (int susNote = 0; susNote < static_cast<int>(susLength); susNote++) {
                    oldNote = spawnNotes.back();

                    Note* sustainNote = new Note(strum->getX(), strum->getY(), daNoteData, 
                                               daStrumTime + (Conductor::stepCrochet * susNote) + Conductor::stepCrochet,
                                               GameConfig::getInstance()->getNoteskin(), true, keyCount);
                    sustainNote->lastNote = oldNote;
                    sustainNote->isSustainNote = true;

                    if (susNote == static_cast<int>(susLength) - 1) {
                        sustainNote->isEndNote = true;
                        sustainNote->playAnim("holdend");
                    } else {
                        sustainNote->playAnim("hold");
                    }

                    oldNote->nextNote = sustainNote;
                    spawnNotes.push_back(sustainNote);
                totalNotes++;
                }
            }
        }
    }

    Log::getInstance().info("Generated " + std::to_string(totalNotes) + " total notes");
    Log::getInstance().info("Sorting " + std::to_string(spawnNotes.size()) + " notes by strumTime");

    std::sort(spawnNotes.begin(), spawnNotes.end(), 
        [](Note* a, Note* b) { return a->strumTime < b->strumTime; });

    unspawnNotes = spawnNotes;
    Log::getInstance().info("Final unspawnNotes count: " + std::to_string(unspawnNotes.size()));
}

void PlayState::updateAccuracy() {
    totalPlayed += 1;
    accuracy = (totalNotesHit / totalPlayed) * 100;
    if (accuracy >= 100.00f) {
        if (pfc && misses == 0)
            accuracy = 100.00f;
        else {
            accuracy = 99.98f;
            pfc = false;
        }
    }
}

void PlayState::updateRank() {
    if (accuracy == 100.00f)
        curRank = "P";
    else if (accuracy >= 90.00f)
        curRank = "A";
    else if (accuracy >= 80.00f)
        curRank = "B";
    else if (accuracy >= 70.00f)
        curRank = "C";
    else if (accuracy >= 60.00f)
        curRank = "D";
    else
        curRank = "F";
}

void PlayState::destroy() {
}

void PlayState::openSubState(SubState* subState) {
    std::cout << "PlayState::openSubState called" << std::endl;
    State::openSubState(subState);
}

void PlayState::updateScoreText() {
    std::string text = "Score: " + std::to_string(score) + " Misses: " + std::to_string(misses);
    scoreText->setText(text);
}

void PlayState::goodNoteHit(Note* note) {
    if (!note->wasGoodHit) {
        note->wasGoodHit = true;
        
        if (note->direction >= 0 && note->direction < 4) {
            int arrowIndex = note->direction + 4;
            if (arrowIndex < strumLineNotes.size() && strumLineNotes[arrowIndex]) {
                float currentX = strumLineNotes[arrowIndex]->getX();
                float currentY = strumLineNotes[arrowIndex]->getY();
                
                strumLineNotes[arrowIndex]->playAnimation("confirm");
                
                strumLineNotes[arrowIndex]->setPosition(currentX, currentY);
            }
        }

        combo++;
        score += 350;
        updateScoreText();
        
        if (!note->isSustainNote) {
            note->kill = true;
        }
        else if (note->isEndNote) {
            note->kill = true;
        }
    }
}

void PlayState::noteMiss(int direction) {
    combo = 0;
    misses++;
    score -= 10;
    if (score < 0) score = 0;
    updateScoreText();
}

void PlayState::loadKeybinds() {
    std::ifstream configFile("assets/config.json");
    if (!configFile.is_open()) {
        Log::getInstance().error("Failed to open config.json");
        return;
    }

    nlohmann::json config;
    try {
        configFile >> config;
    } catch (const std::exception& ex) {
        Log::getInstance().error("Failed to parse config.json: " + std::string(ex.what()));
        return;
    }

    if (config.contains("mainBinds")) {
        auto mainBinds = config["mainBinds"];
        arrowKeys[0].primary = getScancodeFromString(mainBinds["left"].get<std::string>());
        arrowKeys[1].primary = getScancodeFromString(mainBinds["down"].get<std::string>());
        arrowKeys[2].primary = getScancodeFromString(mainBinds["up"].get<std::string>());
        arrowKeys[3].primary = getScancodeFromString(mainBinds["right"].get<std::string>());
    }

    if (config.contains("altBinds")) {
        auto altBinds = config["altBinds"];
        arrowKeys[0].alternate = getScancodeFromString(altBinds["left"].get<std::string>());
        arrowKeys[1].alternate = getScancodeFromString(altBinds["down"].get<std::string>());
        arrowKeys[2].alternate = getScancodeFromString(altBinds["up"].get<std::string>());
        arrowKeys[3].alternate = getScancodeFromString(altBinds["right"].get<std::string>());
    }

    if (config.contains("nxBinds")) {
        auto nxBinds = config["nxBinds"];
        nxArrowKeys[0].primary = getButtonFromString(nxBinds["left"].get<std::string>());
        nxArrowKeys[1].primary = getButtonFromString(nxBinds["down"].get<std::string>());
        nxArrowKeys[2].primary = getButtonFromString(nxBinds["up"].get<std::string>());
        nxArrowKeys[3].primary = getButtonFromString(nxBinds["right"].get<std::string>());
    }

    if (config.contains("nxAltBinds")) {
        auto nxAltBinds = config["nxAltBinds"];
        nxArrowKeys[0].alternate = getButtonFromString(nxAltBinds["left"].get<std::string>());
        nxArrowKeys[1].alternate = getButtonFromString(nxAltBinds["down"].get<std::string>());
        nxArrowKeys[2].alternate = getButtonFromString(nxAltBinds["up"].get<std::string>());
        nxArrowKeys[3].alternate = getButtonFromString(nxAltBinds["right"].get<std::string>());
    }
}

SDL_Scancode PlayState::getScancodeFromString(const std::string& keyName) {
    static const std::map<std::string, SDL_Scancode> keyMap = {
        {"A", SDL_SCANCODE_A},
        {"B", SDL_SCANCODE_B},
        {"C", SDL_SCANCODE_C},
        {"D", SDL_SCANCODE_D},
        {"E", SDL_SCANCODE_E},
        {"F", SDL_SCANCODE_F},
        {"G", SDL_SCANCODE_G},
        {"H", SDL_SCANCODE_H},
        {"I", SDL_SCANCODE_I},
        {"J", SDL_SCANCODE_J},
        {"K", SDL_SCANCODE_K},
        {"L", SDL_SCANCODE_L},
        {"M", SDL_SCANCODE_M},
        {"N", SDL_SCANCODE_N},
        {"O", SDL_SCANCODE_O},
        {"P", SDL_SCANCODE_P},
        {"Q", SDL_SCANCODE_Q},
        {"R", SDL_SCANCODE_R},
        {"S", SDL_SCANCODE_S},
        {"T", SDL_SCANCODE_T},
        {"U", SDL_SCANCODE_U},
        {"V", SDL_SCANCODE_V},
        {"W", SDL_SCANCODE_W},
        {"X", SDL_SCANCODE_X},
        {"Y", SDL_SCANCODE_Y},
        {"Z", SDL_SCANCODE_Z},
        {"ArrowLeft", SDL_SCANCODE_LEFT},
        {"ArrowRight", SDL_SCANCODE_RIGHT},
        {"ArrowUp", SDL_SCANCODE_UP},
        {"ArrowDown", SDL_SCANCODE_DOWN},
        {"Space", SDL_SCANCODE_SPACE},
        {"Enter", SDL_SCANCODE_RETURN},
        {"Escape", SDL_SCANCODE_ESCAPE},
        {"Left", SDL_SCANCODE_LEFT},
        {"Right", SDL_SCANCODE_RIGHT},
        {"Up", SDL_SCANCODE_UP},
        {"Down", SDL_SCANCODE_DOWN}
    };

    auto it = keyMap.find(keyName);
    if (it != keyMap.end()) {
        return it->second;
    }

    Log::getInstance().warning("Unknown key name: " + keyName);
    return SDL_SCANCODE_UNKNOWN;
}

SDL_GameControllerButton PlayState::getButtonFromString(const std::string& buttonName) {
    static const std::map<std::string, SDL_GameControllerButton> buttonMap = {
        {"A", SDL_CONTROLLER_BUTTON_A},
        {"B", SDL_CONTROLLER_BUTTON_B},
        {"X", SDL_CONTROLLER_BUTTON_X},
        {"Y", SDL_CONTROLLER_BUTTON_Y},
        {"DPAD_LEFT", SDL_CONTROLLER_BUTTON_DPAD_LEFT},
        {"DPAD_RIGHT", SDL_CONTROLLER_BUTTON_DPAD_RIGHT},
        {"DPAD_UP", SDL_CONTROLLER_BUTTON_DPAD_UP},
        {"DPAD_DOWN", SDL_CONTROLLER_BUTTON_DPAD_DOWN},
        {"LEFT_SHOULDER", SDL_CONTROLLER_BUTTON_LEFTSHOULDER},
        {"RIGHT_SHOULDER", SDL_CONTROLLER_BUTTON_RIGHTSHOULDER},
        {"LEFT_TRIGGER", SDL_CONTROLLER_BUTTON_LEFTSHOULDER},
        {"RIGHT_TRIGGER", SDL_CONTROLLER_BUTTON_RIGHTSHOULDER},
        {"ZL", SDL_CONTROLLER_BUTTON_LEFTSHOULDER},
        {"ZR", SDL_CONTROLLER_BUTTON_RIGHTSHOULDER},
        {"LT", SDL_CONTROLLER_BUTTON_LEFTSHOULDER},
        {"RT", SDL_CONTROLLER_BUTTON_RIGHTSHOULDER},
        {"LEFT_STICK", SDL_CONTROLLER_BUTTON_LEFTSTICK},
        {"RIGHT_STICK", SDL_CONTROLLER_BUTTON_RIGHTSTICK},
        {"START", SDL_CONTROLLER_BUTTON_START},
        {"BACK", SDL_CONTROLLER_BUTTON_BACK}
    };

    auto it = buttonMap.find(buttonName);
    if (it != buttonMap.end()) {
        return it->second;
    }

    Log::getInstance().warning("Unknown button name: " + buttonName);
    return SDL_CONTROLLER_BUTTON_INVALID;
}

void PlayState::handleOpponentNoteHit(float deltaTime) {
    static float animationTimer = 0.0f;
    static bool isAnimating = false;
    static int currentArrowIndex = -1;

    for (auto note : notes) {
        if (note && !note->shouldHit && !note->wasGoodHit) {
            float timeDiff = note->strumTime - Conductor::songPosition;
            
            if (timeDiff <= 45.0f && timeDiff >= -Conductor::safeZoneOffset) {
                note->canBeHit = true;
                
                int arrowIndex = note->direction;
                if (arrowIndex < strumLineNotes.size() && strumLineNotes[arrowIndex]) {
                    strumLineNotes[arrowIndex]->playAnimation("confirm");
                    isAnimating = true;
                    currentArrowIndex = arrowIndex;
                    animationTimer = 0.0f;
                }
                
                note->wasGoodHit = true;
                note->kill = true;
            }
        }
    }

    if (isAnimating) {
        animationTimer += deltaTime;
        if (animationTimer >= 0.1f) {
            if (currentArrowIndex >= 0 && currentArrowIndex < strumLineNotes.size() && strumLineNotes[currentArrowIndex]) {
                strumLineNotes[currentArrowIndex]->playAnimation("static");
            }
            isAnimating = false;
            currentArrowIndex = -1;
        }
    }
}