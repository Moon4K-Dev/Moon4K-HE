#include "FreeplayStateLegacy.h"
#include "FreeplayState.h"
#include "PlayState.h"
#include "MainMenuState.h"
#include <random>
#include <algorithm>
#include <fstream>
#include "../backend/json.hpp"

using json = nlohmann::json;

FreeplayStateLegacy* FreeplayStateLegacy::instance = nullptr;

FreeplayStateLegacy::FreeplayStateLegacy() 
    : curSelected(0)
    , scoreText(nullptr)
    , missText(nullptr)
    , diffText(nullptr)
    , noSongsText(nullptr)
    , songImage(nullptr)
    , menuBG(nullptr)
    , songHeight(100.0f) {
    instance = this;
    loadSongs();
}

FreeplayStateLegacy::~FreeplayStateLegacy() {
    destroy();
}

void FreeplayStateLegacy::create() {
    SwagState::create();

    Engine* engine = Engine::getInstance();
    
    menuBG = new Sprite(Paths::image("mainmenu/menubglol"));
    menuBG->setAlpha(0.7f);
    engine->addSprite(menuBG);

    SDL_SetRenderDrawBlendMode(SDLManager::getInstance().getRenderer(), SDL_BLENDMODE_BLEND);
    
    Text* helpText = new Text();
    helpText->setFormat(Paths::font("vcr.ttf"), 18, 0xFFFFFFFF);
    helpText->setText("Press R to scan the songs folder.");
    helpText->setPosition(4, engine->getWindowHeight() - 26);
    engine->addText(helpText);

    songImage = new Sprite();
    songImage->setScale(0.45f, 0.45f);
    songImage->setPosition(
        engine->getWindowWidth() * 0.5f,
        100
    );
    engine->addSprite(songImage);

    scoreText = new Text();
    scoreText->setFormat(Paths::font("vcr.ttf"), 22, 0xFFFFFFFF);
    scoreText->setPosition(engine->getWindowWidth() * 0.7f, 5);
    engine->addText(scoreText);

    missText = new Text();
    missText->setFormat(Paths::font("vcr.ttf"), 22, 0xFFFFFFFF);
    missText->setPosition(engine->getWindowWidth() * 0.7f, 40);
    engine->addText(missText);

    diffText = new Text();
    diffText->setFormat(Paths::font("vcr.ttf"), 24, 0xFFFFFFFF);
    diffText->setPosition(scoreText->getX(), scoreText->getY() + 36);
    engine->addText(diffText);

    updateSongList();
    changeSelection();

    Discord::GetInstance().SetDetails("Selecting a song...(in the FreeplayState - Legacy Version!)");
    Discord::GetInstance().Update();
}

void FreeplayStateLegacy::update(float deltaTime) {
    SwagState::update(deltaTime);
    Input::UpdateKeyStates();
    Input::UpdateControllerStates();

    if (!isTransitioning()) {
        if (Input::justPressed(SDL_SCANCODE_BACKSPACE) || Input::justPressed(SDL_SCANCODE_ESCAPE)) {
            startTransitionOut(0.5f);
            Engine::getInstance()->switchState(new MainMenuState());
        }

        if (Input::justPressed(SDL_SCANCODE_UP) || Input::justPressed(SDL_SCANCODE_DOWN)) {
            changeSelection(Input::justPressed(SDL_SCANCODE_UP) ? -1 : 1);
            updateSongImage();
        }

        if (Input::justPressed(SDL_SCANCODE_RETURN)) {
            if (!songs.empty() && curSelected >= 0 && curSelected < static_cast<int>(songs.size())) {
                SoundManager::getInstance().stopMusic();
                startTransitionOut(0.5f);
                std::string selectedSong = songs[curSelected];
                Engine::getInstance()->switchState(new PlayState(selectedSong));
            }
        }

        if (Input::justPressed(SDL_SCANCODE_SPACE)) {
            Engine::getInstance()->switchState(new FreeplayState());
        }

        if (Input::justPressed(SDL_SCANCODE_R)) {
            rescanSongs();
        }
    }
}

void FreeplayStateLegacy::render() {
    if (menuBG) {
        menuBG->render();
    }

    SDL_Rect scoreBG = {
        static_cast<int>(scoreText->getX() - 6),
        0,
        static_cast<int>(Engine::getInstance()->getWindowWidth() * 0.35f),
        66
    };
    SDL_SetRenderDrawColor(SDLManager::getInstance().getRenderer(), 0, 0, 0, 153);
    SDL_RenderFillRect(SDLManager::getInstance().getRenderer(), &scoreBG);

    SDL_Rect textBG = {
        0,
        static_cast<int>(Engine::getInstance()->getWindowHeight() - 26),
        Engine::getInstance()->getWindowWidth(),
        26
    };
    SDL_SetRenderDrawColor(SDLManager::getInstance().getRenderer(), 0, 0, 0, 153);
    SDL_RenderFillRect(SDLManager::getInstance().getRenderer(), &textBG);

    if (songImage) {
        songImage->render();
    }

    for (auto text : songTexts) {
        text->render();
    }

    if (scoreText) scoreText->render();
    if (missText) missText->render();
    if (diffText) diffText->render();
    if (noSongsText) noSongsText->render();

    SwagState::render();
}

void FreeplayStateLegacy::destroy() {
    songImage = nullptr;
    menuBG = nullptr;
    scoreText = nullptr;
    missText = nullptr;
    diffText = nullptr;
}

void FreeplayStateLegacy::changeSelection(int change) {
    curSelected += change;

    if (curSelected < 0) {
        curSelected = songs.size() - 1;
    }
    if (curSelected >= static_cast<int>(songs.size())) {
        curSelected = 0;
    }

    float startY = 50.0f;
    float spacing = songHeight;

    float offsetY = std::max<float>(0.0f, (curSelected * spacing) - (Engine::getInstance()->getWindowHeight() / 2.0f) + (spacing / 2.0f));

    for (size_t i = 0; i < songTexts.size(); i++) {
        Text* txt = songTexts[i];
        txt->setPosition(20, startY + (i * spacing) - offsetY);
        txt->setAlpha(i == static_cast<size_t>(curSelected) ? 1.0f : 0.7f);
        
        if (i == static_cast<size_t>(curSelected)) {
            txt->setFormat(Paths::font("vcr.ttf"), 36, 0xFFFFFFFF);
        } else {
            txt->setFormat(Paths::font("vcr.ttf"), 32, 0xFFFFFFFF);
        }
    }

    if (!songs.empty()) {
        selectedSong = songs[curSelected];
        
        // TODO: highscore shit
        scoreText->setText("SCORE: 0");
        missText->setText("MISSES: 0");
        diffText->setText("");
    }
}

void FreeplayStateLegacy::updateSongList() {
    for (auto text : songTexts) {
        delete text;
    }
    songTexts.clear();
    
    loadSongs();

    if (songs.empty()) {
        if (!noSongsText) {
            noSongsText = new Text();
            noSongsText->setFormat(Paths::font("vcr.ttf"), 24, 0xFF0000FF);
            noSongsText->setText("There's no songs in the songs folder!");
            noSongsText->setPosition(
                (Engine::getInstance()->getWindowWidth() - noSongsText->getWidth()) / 2,
                Engine::getInstance()->getWindowHeight() / 2 - 10
            );
            Engine::getInstance()->addText(noSongsText);
        }
    } else {
        if (noSongsText) {
            delete noSongsText;
            noSongsText = nullptr;
        }

        for (size_t i = 0; i < songs.size(); i++) {
            Text* songText = new Text();
            songText->setFormat(Paths::font("vcr.ttf"), 32, 0xFFFFFFFF);
            songText->setText(songs[i]);
            songText->setPosition(20, 50 + (i * songHeight));
            Engine::getInstance()->addText(songText);
            songTexts.push_back(songText);
        }

        selectedSong = songs[curSelected];
        updateSongImage();
    }
}

void FreeplayStateLegacy::rescanSongs() {
    updateSongList();
    changeSelection();
}

void FreeplayStateLegacy::updateSongImage() {
    if (selectedSong.empty()) return;

    std::string imagePath = "assets/charts/" + selectedSong + "/image.png";
    if (!std::filesystem::exists(imagePath)) {
        imagePath = "assets/charts/" + selectedSong + "/" + selectedSong + "-bg.png";
    }
    if (!std::filesystem::exists(imagePath)) {
        imagePath = "assets/charts/" + selectedSong + "/" + selectedSong + ".png";
    }

    if (std::filesystem::exists(imagePath)) {
        songImage->loadTexture(imagePath);
        songImage->setScale(0.45f, 0.45f);
        songImage->setPosition(
            Engine::getInstance()->getWindowWidth() * 0.5f,
            100
        );
    } else {
        SDL_Surface* surface = SDL_CreateRGBSurface(0, 400, 300, 32, 0, 0, 0, 0);
        SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 0, 0));
        SDL_Texture* texture = SDL_CreateTextureFromSurface(SDLManager::getInstance().getRenderer(), surface);
        SDL_FreeSurface(surface);
        songImage->setTexture(texture);
    }
}

void FreeplayStateLegacy::loadSongJson(const std::string& songName) {
    std::string path = "assets/charts/" + songName + "/" + songName + ".moon";
    std::transform(path.begin(), path.end(), path.begin(), ::tolower);

    std::ifstream file(path);
    if (!file.is_open()) {
        Log::getInstance().error("Could not open song file: " + path);
        return;
    }

    try {
        json j;
        file >> j;
        
        songData.song = j["song"].get<std::string>();
        songData.bpm = j["bpm"].get<float>();
        songData.speed = j.value("speed", 1.0f);
        songData.sections = j["sections"].get<int>();
        songData.keyCount = j.value("keyCount", 4);
        songData.validScore = true;

        if (j.contains("notes") && j["notes"].is_array()) {
            songData.notes.clear();
            
            for (const auto& sectionJson : j["notes"]) {
                SongSection section;
                section.mustHitSection = sectionJson.value("mustHitSection", true);
                section.bpm = sectionJson.value("bpm", songData.bpm);

                if (sectionJson.contains("sectionNotes") && sectionJson["sectionNotes"].is_array()) {
                    for (const auto& noteArray : sectionJson["sectionNotes"]) {
                        if (noteArray.is_array() && noteArray.size() >= 2) {
                            SongNote note;
                            note.strumTime = noteArray[0].get<float>();
                            note.direction = noteArray[1].get<int>();
                            note.sustainLength = noteArray.size() > 3 ? noteArray[3].get<float>() : 0.0f;
                            
                            if (note.direction < songData.keyCount) {
                                section.sectionNotes.push_back(note);
                            }
                        }
                    }
                }
                
                songData.notes.push_back(section);
            }
        }

        if (scoreText) {
            int totalNotes = 0;
            for (const auto& section : songData.notes) {
                totalNotes += section.sectionNotes.size();
            }
            scoreText->setText("NOTES: " + std::to_string(totalNotes));
        }
        if (diffText) {
            diffText->setText("BPM: " + std::to_string(static_cast<int>(songData.bpm)));
        }
        Log::getInstance().info("Loaded song: " + songName);
        Log::getInstance().info("Song data: " + songData.song);
        Log::getInstance().info("Song bpm: " + std::to_string(songData.bpm));
        Log::getInstance().info("Song speed: " + std::to_string(songData.speed));
        Log::getInstance().info("Song sections: " + std::to_string(songData.sections));
        Log::getInstance().info("Song key count: " + std::to_string(songData.keyCount));        
        
    } catch (const std::exception& e) {
        Log::getInstance().error("Error parsing song JSON: " + std::string(e.what()));
        songData.validScore = false;
    }
}

void FreeplayStateLegacy::loadSongs() {
    songs.clear();
    std::string chartsDir = "assets/charts/";
    
    if (!std::filesystem::exists(chartsDir)) {
        Log::getInstance().warning("Charts directory does not exist: " + chartsDir);
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(chartsDir)) {
        if (entry.is_directory()) {
            songs.push_back(entry.path().filename().string());
        }
    }

    std::sort(songs.begin(), songs.end());
}
