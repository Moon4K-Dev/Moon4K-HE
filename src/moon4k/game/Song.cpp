#include "Song.h"
#include "../backend/json.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>
#include "../../engine/utils/Log.h"

using json = nlohmann::json;

Song::Song(const std::string& song, const std::vector<SwagSection>& notes, float bpm, int sections, int keyCount, const std::vector<int>& timescale)
    : song(song), notes(notes), bpm(bpm), sections(sections), keyCount(keyCount), timescale(timescale) {
    speed = 1.0f;
    for (size_t i = 0; i < notes.size(); i++) {
        sectionLengths.push_back(notes[i]);
    }
}

SwagSong Song::loadFromJson(const std::string& songName, const std::string& folder) {
    std::string actualFolder = folder;
    std::string lowerFolder = actualFolder;
    std::string lowerSongName = songName;
    
    std::transform(lowerFolder.begin(), lowerFolder.end(), lowerFolder.begin(), ::tolower);
    std::transform(lowerSongName.begin(), lowerSongName.end(), lowerSongName.begin(), ::tolower);
    
    std::string path = "assets/charts/";
    if (!lowerFolder.empty()) {
        path += lowerFolder + "/";
    }
    path += lowerSongName + ".moon";

    std::cout << "Final path: " << path << std::endl;

    std::string rawJson;
    std::ifstream file(path);
    if (!file.is_open()) {
        Log::getInstance().error("Could not open file: " + path);
        return SwagSong();
    }
    
    rawJson = std::string(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );
    file.close();

    while (!rawJson.empty() && std::isspace(rawJson.back())) {
        rawJson.pop_back();
    }

    while (!rawJson.empty() && rawJson.back() != '}') {
        rawJson.pop_back();
    }

    return parseJSONshit(rawJson);
}

SwagSong Song::parseJSONshit(const std::string& rawJson) {
    SwagSong swagShit;
    try {
        json j = json::parse(rawJson);
        
        json songData = j;
        if (j.contains("song") && j["song"].is_object()) {
            songData = j["song"];
        }

        if (songData.contains("song")) {
            if (songData["song"].is_string()) {
                swagShit.song = songData["song"].get<std::string>();
            } else if (songData["song"].is_object() && songData["song"].contains("name")) {
                swagShit.song = songData["song"]["name"].get<std::string>();
            }
        }

        swagShit.bpm = songData.value("bpm", 100.0f);
        swagShit.speed = songData.value("speed", 1.0f);
        swagShit.sections = songData.value("sections", 0);
        swagShit.keyCount = songData.value("keyCount", 4);
        
        if (songData.contains("timescale") && songData["timescale"].is_array()) {
            for (const auto& value : songData["timescale"]) {
                if (value.is_number()) {
                    swagShit.timescale.push_back(value.get<int>());
                }
            }
        }
        
        if (songData.contains("notes") && songData["notes"].is_array()) {
            for (const auto& noteJson : songData["notes"]) {
                SwagSection section;
                
                if (noteJson.is_object()) {
                    section.lengthInSteps = noteJson.value("lengthInSteps", 16);
                    section.mustHitSection = noteJson.value("mustHitSection", true);
                    section.typeOfSection = noteJson.value("typeOfSection", 0);
                    section.bpm = noteJson.value("bpm", 0.0f);
                    section.changeBPM = noteJson.value("changeBPM", false);
                    section.altAnim = noteJson.value("altAnim", false);
                    
                    if (noteJson.contains("sectionNotes") && noteJson["sectionNotes"].is_array()) {
                        for (const auto& noteData : noteJson["sectionNotes"]) {
                            if (noteData.is_object()) {
                                float strumTime = noteData.value("noteStrum", 0.0f);
                                int noteVal = noteData.value("noteData", 0);
                                float susLength = noteData.value("noteSus", 0.0f);
                                
                                std::vector<float> note = {strumTime, static_cast<float>(noteVal), 0.0f, susLength};
                                section.sectionNotes.push_back(note);
                                
                                //Log::getInstance().info("Added note: strumTime=" + std::to_string(strumTime) + ", noteData=" + std::to_string(noteVal) + ", susLength=" + std::to_string(susLength));
                            }
                            else if (noteData.is_array()) {
                                std::vector<float> note;
                                for (const auto& value : noteData) {
                                    if (value.is_number()) {
                                        note.push_back(value.get<float>());
                                    } else if (value.is_string()) {
                                        try {
                                            note.push_back(std::stof(value.get<std::string>()));
                                        } catch (...) {
                                            continue;
                                        }
                                    }
                                }
                                if (!note.empty()) {
                                    section.sectionNotes.push_back(note);
                                }
                            }
                        }
                    }
                }
                
                swagShit.notes.push_back(section);
                swagShit.sectionLengths.push_back(section);
            }
        }

        if (!swagShit.song.empty() && swagShit.bpm > 0) {
            swagShit.validScore = true;
        }

        Log::getInstance().info("Parsed song: " + swagShit.song);
        Log::getInstance().info("BPM: " + std::to_string(swagShit.bpm));
        Log::getInstance().info("Key Count: " + std::to_string(swagShit.keyCount));
        Log::getInstance().info("Sections: " + std::to_string(swagShit.sections));
        Log::getInstance().info("Notes count: " + std::to_string(swagShit.notes.size()));

    } catch (const json::exception& ex) {
        Log::getInstance().error("JSON parsing error: " + std::string(ex.what()));
        return SwagSong();
    }

    return swagShit;
} 