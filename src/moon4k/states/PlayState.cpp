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
#include "FreeplayState.h"
#include "../substates/ResultsSubState.h"
#include <filesystem>
#include "../game/ScoreManager.h"

PlayState* PlayState::instance = nullptr;
SwagSong PlayState::SONG;
Sound* PlayState::inst = nullptr;
Sound* PlayState::voices = nullptr;
VideoPlayer* PlayState::videoPlayer = nullptr;

PlayState::PlayState(const std::string& songName, const std::string& difficulty)
    : SwagState()
    , directSongName(songName)
    , selectedDifficulty(difficulty)
    , countdownText(nullptr)
    , loadingText(nullptr)
{
    instance = this;
    Note::loadAssets();
    
    countdownText = new Text();
    countdownText->setFormat("assets/fonts/vcr.ttf", 64, 0xFFFFFFFF);
    
    loadingText = new Text();
    loadingText->setFormat("assets/fonts/vcr.ttf", 32, 0xFFFFFFFF);
    loadingText->setText("Loading chart...");
    
    int windowWidth = Engine::getInstance()->getWindowWidth();
    int windowHeight = Engine::getInstance()->getWindowHeight();
    countdownText->setPosition(static_cast<float>(windowWidth / 2 - 20), static_cast<float>(windowHeight / 2 - 32));
    loadingText->setPosition(static_cast<float>(windowWidth / 2 - 50), static_cast<float>(windowHeight / 2 - 16));

    ui = std::make_unique<UI>();
    loadKeybinds();
}

PlayState::~PlayState() {
    if (inst != nullptr) {
        delete inst;
        inst = nullptr;
    }
    
    if (voices != nullptr) {
        delete voices;
        voices = nullptr;
    }

    for (auto arrow : strumLineNotes) {
        delete arrow;
    }
    strumLineNotes.clear();

    for (auto note : notes) {
        delete note;
    }
    notes.clear();

    delete countdownText;
    delete loadingText;
    Note::unloadAssets();
    destroy();
}

void PlayState::create() {
    Engine* engine = Engine::getInstance();
    videoPlayer = new VideoPlayer();
    generateSong(directSongName);
    startingSong = true;
}

void PlayState::checkAndSetBackground() {
    std::string daSongswag = SONG.song;
    std::string imagePath = "assets/charts/" + daSongswag + "/image.png";
    std::string imagePathAlt = "assets/charts/" + daSongswag + "/" + daSongswag + "-bg.png";
    std::string imagePathAlt2 = "assets/charts/" + daSongswag + "/" + daSongswag + ".png";
    std::string videoPath = "assets/charts/" + daSongswag + "/video.mp4";

    songBG = std::make_unique<Sprite>();
    bool bgLoaded = false;

    if (Paths::exists(imagePath)) {
        songBG->loadTexture(imagePath);
        bgLoaded = true;
    }
    else if (!bgLoaded && Paths::exists(imagePathAlt)) {
        songBG->loadTexture(imagePathAlt);
        bgLoaded = true;
    }
    else if (!bgLoaded && Paths::exists(imagePathAlt2)) {
        songBG->loadTexture(imagePathAlt2);
        bgLoaded = true;
    }
    else if (!bgLoaded && Paths::exists(videoPath)) {
        loadVideo(videoPath);
        setVolume(0); // ehh.... idk if I should keep this at 0 or maybe 20?? 30?? 
        bgLoaded = true;
    }

    if (bgLoaded) {
        songBG->setScale(1.1f, 1.1f);
        int windowWidth = Engine::getInstance()->getWindowWidth();
        int windowHeight = Engine::getInstance()->getWindowHeight();
        songBG->setPosition(
            (windowWidth - songBG->getWidth() * songBG->getScale().x) / 2.0f,
            (windowHeight - songBG->getHeight() * songBG->getScale().y) / 2.0f
        );
        songBG->setVisible(true);
        songBG->setAlpha(1.0f);
        Log::getInstance().info("Loaded song background: " + daSongswag);
        return;
    }

    Log::getInstance().info("No background found for: " + daSongswag);
    if (songBG) songBG->setVisible(false);
}

void PlayState::update(float deltaTime) {
    SwagState::update(deltaTime);

    #ifdef __SWITCH__
    // nun
    #elif defined(__MINGW32__)
    // nun
    #else
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "In Game - %s - Score: %d - Misses: %d - Accuracy: %.2f%%", curSong.c_str(), score, misses, accuracy);
    Discord::GetInstance().SetDetails(buffer);
    Discord::GetInstance().Update();
    #endif

    if (videoPlayer) {
        videoPlayer->update();
    }

    if (isLoading) {
        if (!unspawnNotes.empty() && inst != nullptr) {
            isLoading = false;
            isCountingDown = true;
            Log::getInstance().info("Chart loaded, starting countdown");
        }
        return;
    }

    if (isCountingDown) {
        countdownTime -= deltaTime;
        int currentCount = static_cast<int>(std::ceil(countdownTime));
        if (currentCount <= 0) {
            isCountingDown = false;
            countdownText->setText("");
            startedCountdown = true;
        } else {
            countdownText->setText(std::to_string(currentCount));
        }
        return;
    }

    if (!_subStates.empty()) {
        _subStates.back()->update(deltaTime);
    } 
    else {
        if (inst && !inst->isPlaying() && !startingSong && !isLoading && !isCountingDown) {
            endSong();
            return;
        }

        Input::UpdateKeyStates();
        Input::UpdateControllerStates();

        handleInput();
        handleOpponentNoteHit(deltaTime);
        updateArrowAnimations();
        updateHealth(deltaTime);

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

            float yOffset = note->isSustainNote ? -14.09f : 0.0f;

            if (GameConfig::getInstance()->getDownscroll()) {
                if (note->isSustainNote) {
                    yOffset -= 25.0f;
                }
                note->setY(strum->getY() + yOffset + (0.45f * (Conductor::songPosition - note->strumTime) * GameConfig::getInstance()->getScrollSpeed()));
                if (note->isSustainNote) {
                    note->setY(note->getY() - note->getHeight() * 2);
                    if (note->isEndNote) {
                        note->setY(note->getY() + note->getHeight() * 0.7f);
                    }
                }
            } else {
                note->setY(strum->getY() + yOffset - (0.45f * (Conductor::songPosition - note->strumTime) * GameConfig::getInstance()->getScrollSpeed()));
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

        if (Input::justPressed(SDL_SCANCODE_SPACE)) {
            endSong();
        }

        ui->setScore(score);
        ui->setNotesHit(combo);
        ui->setAccuracy(accuracy / 100.0f);
        ui->setHealth(health);
        if (inst && inst->isPlaying()) {
            float currentTime = Conductor::songPosition / 1000.0f;
            float totalTime = inst->getDuration();
            ui->setSongTime(currentTime, totalTime);
        }
        ui->update(deltaTime);
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
                        
                        if (note->isSustainNote) {
                            auto strum = strumLineNotes[i];
                            if (strum) {
                                float strumY = strum->getY();
                                float noteY = note->getY();
                                float originalHeight = note->getHeight();
                                
                                if (GameConfig::getInstance()->getDownscroll()) {
                                    float distance = std::abs(noteY - strumY);
                                    float scale = distance / originalHeight;
                                    note->setScale(0.5f, scale);
                                    note->setY(strumY);
                                    
                                    if (note->nextNote) {
                                        note->nextNote->setY(strumY + distance);
                                    }
                                } else {
                                    float distance = std::abs(strumY - noteY);
                                    float scale = distance / originalHeight;
                                    note->setScale(0.5f, scale);
                                    note->setY(strumY);
                                }
                            }
                        }
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
        
        std::string smPath = "assets/charts/" + baseSongName + "/" + baseSongName + ".sm";
        if (std::filesystem::exists(smPath)) {
            SONG = Song::loadFromJson(songName, folder, selectedDifficulty);
            if (!SONG.validScore) {
                Log::getInstance().error("Failed to load StepMania chart");
                throw std::runtime_error("Failed to load StepMania chart");
            }
        } else {
            SONG = Song::loadFromJson(songName, folder);
        }

        if (!SONG.validScore) {
            throw std::runtime_error("Invalid score data");
        }

        Conductor::changeBPM(SONG.bpm);
        curSong = songName;
        sections = SONG.sections;
        sectionLengths = SONG.sectionLengths;
        keyCount = SONG.keyCount ? SONG.keyCount : 4;
        timescale = SONG.timescale;

        if (inst != nullptr) {
            delete inst;
            inst = nullptr;
        }

        std::string instPath = "assets/charts/" + baseSongName + "/" + baseSongName + "_Inst" + soundExt;
        std::string voicesPath = "assets/charts/" + baseSongName + "/" + baseSongName + "_Voices" + soundExt;
        
        if (!std::filesystem::exists(instPath)) {
            instPath = "assets/charts/" + baseSongName + "/" + baseSongName + soundExt;
        }

        inst = new Sound();
        if (!inst->load(instPath)) {
            Log::getInstance().error("Failed to load song audio: " + instPath);
            delete inst;
            inst = nullptr;
        } else {
            Log::getInstance().info("Loaded instrumental: " + instPath);
            
            if (std::filesystem::exists(voicesPath)) {
                voices = new Sound();
                if (!voices->load(voicesPath)) {
                    Log::getInstance().error("Failed to load voices audio: " + voicesPath);
                    delete voices;
                    voices = nullptr;
                } else {
                    Log::getInstance().info("Loaded voices: " + voicesPath);
                }
            }
        }

        checkAndSetBackground();
        startCountdown();
        generateNotes();
        
    } catch (const std::exception& ex) {
        std::cerr << "Error generating song: " << ex.what() << std::endl;
        Log::getInstance().error("Error generating song: " + std::string(ex.what()));
    }
}

void PlayState::startSong() {
    startingSong = false;
    if (inst != nullptr) {
        inst->play();
        if (voices != nullptr) {
            voices->play();
        }
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
    float startX = (windowWidth / 2.0f) - (totalWidth / 2.0f) - 45;
    
    for (int i = 0; i < keyCount; i++) {
        std::string noteskin = GameConfig::getInstance()->getNoteskin();
        StrumNote* babyArrow = new StrumNote(0, yPos, i, noteskin, keyCount);
        
        float x = startX + (i * laneOffset);
        babyArrow->setPosition(x, yPos);
        strumLineNotes.push_back(babyArrow);
    }
}

void PlayState::render() {
    if (videoPlayer) {
        videoPlayer->render(SDLManager::getInstance().getRenderer());
    }

    if (defaultBG && defaultBG->isVisible()) {
        defaultBG->render();
    }
    if (songBG && songBG->isVisible()) {
        songBG->render();
    }

    if (isLoading) {
        loadingText->render();
        return;
    }

    ui->render();

    for (auto arrow : strumLineNotes) {
        if (arrow && arrow->isVisible()) {
            arrow->render();
        }
    }

    for (auto note : notes) {
        if (note && note->isVisible() && note->isSustainNote) {
            note->render();
        }
    }

    for (auto note : notes) {
        if (note && note->isVisible() && !note->isSustainNote) {
            note->render();
        }
    }
    
    if (isCountingDown) {
        countdownText->render();
    }

    static int lastNoteCount = 0;
    if (notes.size() != lastNoteCount) {
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
                    for (int susNote = 0; susNote < static_cast<int>(susLength); susNote++) {
                        oldNote = spawnNotes.back();

                        Note* sustainNote = new Note(strum->getX() - -30.0f, strum->getY() - 10.0f, daNoteData, 
                                                   daStrumTime + (Conductor::stepCrochet * susNote) + (Conductor::stepCrochet / 2),
                                                   GameConfig::getInstance()->getNoteskin(), true, keyCount);
                        sustainNote->lastNote = oldNote;
                        sustainNote->isSustainNote = true;
                        sustainNote->setScale(0.5f, 1.0f);

                        if (susNote == static_cast<int>(susLength) - 1) {
                            sustainNote->isEndNote = true;
                            sustainNote->playAnim("holdend");
                            sustainNote->setScale(0.5f, 0.7f);
                            if (GameConfig::getInstance()->getDownscroll()) {
                                sustainNote->offset.y = -250.0f;
                                sustainNote->setY(sustainNote->getY() - sustainNote->getHeight() * 2);
                            }
                        } else {
                            sustainNote->playAnim("hold");
                            sustainNote->scale.y *= 305.0f * SONG.speed; // why the fuck does it have to be this for it to connect.
                            sustainNote->updateHitbox();
                            if (GameConfig::getInstance()->getDownscroll()) {
                                sustainNote->offset.y = -250.0f;
                                sustainNote->setY(sustainNote->getY() - sustainNote->getHeight() * 2);
                            } else {
                                sustainNote->offset.y = -100.0f;
                            }
                        }

                        oldNote->nextNote = sustainNote;
                        spawnNotes.push_back(sustainNote);
                        totalNotes++;
                    }
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
    playVideo();
}

void PlayState::updateAccuracy() {
    totalPlayed += 1;
    accuracy = totalPlayed > 0 ? (static_cast<float>(totalNotesHit) / static_cast<float>(totalPlayed)) * 100.0f : 0.0f;
    if (accuracy >= 100.00f) {
        if (pfc && misses == 0)
            accuracy = 100.00f;
        else {
            accuracy = 99.98f;
            pfc = false;
        }
    }
    updateRank();
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
    score = 0;
    accuracy = 0.0f;
    misses = 0;
    combo = 0;
    totalPlayed = 0;
    totalNotesHit = 0;
    
    if (voices != nullptr) {
        voices->stop();
        delete voices;
        voices = nullptr;
    }
    if (videoPlayer) {
        videoPlayer = nullptr;
    }
}

void PlayState::openSubState(SubState* subState) {
    std::cout << "PlayState::openSubState called" << std::endl;
    State::openSubState(subState);
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
        if (!note->isSustainNote) {
            totalNotesHit++;
            updateAccuracy();
            gainHealth();
        } else {
            gainHealth();
            
            if (note->nextNote && note->nextNote->isSustainNote) {
                float strumY = strumLineNotes[note->direction]->getY();
                float noteY = note->getY();
                float originalHeight = note->getHeight();
                
                if (GameConfig::getInstance()->getDownscroll()) {
                    float distance = std::abs(noteY - strumY);
                    float scale = distance / originalHeight;
                    note->setScale(0.5f, scale);
                    note->setY(strumY);
                    
                    if (note->nextNote) {
                        note->nextNote->setY(strumY + distance);
                    }
                } else {
                    float distance = std::abs(strumY - noteY);
                    float scale = distance / originalHeight;
                    note->setScale(0.5f, scale);
                    note->setY(strumY);
                }
            }
        }
        
        if (!note->isSustainNote) {
            note->kill = true;
        } else if (note->isEndNote) {
            note->kill = true;
        }
    }
}

void PlayState::noteMiss(int direction) {
    combo = 0;
    misses++;
    score -= 10;
    if (score < 0) score = 0;
    updateAccuracy();
    loseHealth();
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

void PlayState::updateHealth(float deltaTime) {
    if (isDead) return;

    health -= HEALTH_DRAIN * deltaTime;
    health = std::clamp(health, 0.0f, 1.0f);

    if (health <= 0.0f) {
        isDead = true;
        health = 0.0f;
        // might make an actual game over state later
        if (inst) {
            inst->stop();
            delete inst;
            inst = nullptr;
        }
        if (voices) {
            voices->stop();
            delete voices;
            voices = nullptr;
        }
        SoundManager::getInstance().stopMusic();
        startTransitionOut(0.5f);
        Engine::getInstance()->switchState(new FreeplayState());
    }
}

void PlayState::gainHealth() {
    if (isDead) return;
    health += HEALTH_GAIN;
    health = std::clamp(health, 0.0f, 1.0f);
}

void PlayState::loseHealth() {
    if (isDead) return;
    health -= HEALTH_LOSS;
    health = std::clamp(health, 0.0f, 1.0f);

    if (health <= 0.0f) {
        isDead = true;
        // might make an actual game over state later
        if (inst) {
            inst->stop();
            delete inst;
            inst = nullptr;
        }
        if (voices) {
            voices->stop();
            delete voices;
            voices = nullptr;
        }
        SoundManager::getInstance().stopMusic();
        startTransitionOut(0.5f);
        Engine::getInstance()->switchState(new FreeplayState());
    }
}

void PlayState::endSong() {
    if (inst) {
        inst->stop();
    }
    if (voices) {
        voices->stop();
    }
    
    ScoreManager::getInstance()->saveScore(
        directSongName,
        selectedDifficulty,
        score,
        misses,
        accuracy,
        totalNotesHit,
        curRank
    );
    
    ResultsSubState* resultsSubState = new ResultsSubState();    
    resultsSubState->setStats(score, misses, accuracy, totalNotesHit, curRank);    
    openSubState(resultsSubState);
    Log::getInstance().info("Song ended, showing results");
}

bool PlayState::loadVideo(const std::string& path) {
    if (!videoPlayer) {
        Log::getInstance().error("Video player not initialized");
        return false;
    }

    currentVideoPath = path;
    return videoPlayer->loadVideo(path);
}

void PlayState::playVideo() {
    if (videoPlayer) {
        videoPlayer->play();
    }
}

void PlayState::pauseVideo() {
    if (videoPlayer) {
        videoPlayer->pause();
    }
}

void PlayState::stopVideo() {
    if (videoPlayer) {
        videoPlayer->stop();
    }
}

void PlayState::setVolume(int volume) {
    if (videoPlayer) {
        videoPlayer->setVolume(volume);
    }
}