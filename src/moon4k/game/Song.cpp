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

std::string Song::getFileExtension(const std::string& path) {
    std::string cleanPath = path;
    if (cleanPath.front() == '"' && cleanPath.back() == '"') {
        cleanPath = cleanPath.substr(1, cleanPath.length() - 2);
    }
    
    std::filesystem::path fsPath(cleanPath);
    std::string ext = fsPath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

SwagSong Song::loadFromJson(const std::string& songName, const std::string& folder) {
    std::string actualFolder = folder;
    std::string lowerFolder = actualFolder;
    std::string lowerSongName = songName;
    
    std::transform(lowerFolder.begin(), lowerFolder.end(), lowerFolder.begin(), ::tolower);
    std::transform(lowerSongName.begin(), lowerSongName.end(), lowerSongName.begin(), ::tolower);
    
    std::string basePath = "assets/charts/";
    if (!lowerFolder.empty()) {
        basePath += lowerFolder + "/";
    }

    Log::getInstance().info("Looking for charts in: " + basePath);

    std::string moonPath = basePath + lowerSongName + ".moon";
    if (std::filesystem::exists(moonPath)) {
        Log::getInstance().info("Found Moon4K chart: " + moonPath);
        std::ifstream file(moonPath);
        if (file.is_open()) {
            std::string rawJson((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    while (!rawJson.empty() && std::isspace(rawJson.back())) {
        rawJson.pop_back();
    }
    while (!rawJson.empty() && rawJson.back() != '}') {
        rawJson.pop_back();
    }

    return parseJSONshit(rawJson);
        }
    }

    std::vector<std::pair<std::string, Tsukiyo::Chart::Format>> formats = {
        {".osu", Tsukiyo::Chart::Format::OsuMania},
        {".json", Tsukiyo::Chart::Format::FNFLegacy},
        {".sm", Tsukiyo::Chart::Format::StepMania}
    };

    for (const auto& [ext, format] : formats) {
        std::string chartPath = basePath + lowerSongName + ext;
        Log::getInstance().info("Trying format " + std::string(ext) + ": " + chartPath);
        
        if (std::filesystem::exists(chartPath)) {
            Log::getInstance().info("Found chart file: " + chartPath);
            auto sourceChart = Tsukiyo::Chart::createChart(format);
            if (sourceChart) {
                Log::getInstance().info("Created chart reader for format: " + std::to_string(static_cast<int>(format)));
                if (sourceChart->loadFromFile(chartPath)) {
                    Log::getInstance().info("Successfully loaded chart from: " + chartPath);
                    auto moon4kChart = Tsukiyo::ChartConverter::convert(*sourceChart, Tsukiyo::Chart::Format::Moon4K);
                    if (moon4kChart) {
                        Log::getInstance().info("Successfully converted chart to Moon4K format");
                        return convertTsukiyoToSwagSong(*moon4kChart);
                    } else {
                        Log::getInstance().error("Failed to convert chart to Moon4K format");
                    }
                } else {
                    Log::getInstance().error("Failed to load chart from: " + chartPath);
                }
            } else {
                Log::getInstance().error("Failed to create chart reader for format: " + std::to_string(static_cast<int>(format)));
            }
        }
    }

    Log::getInstance().error("Could not find or load any supported chart format for: " + songName);
    return SwagSong();
}

SwagSong Song::convertTsukiyoToSwagSong(const Tsukiyo::Chart& chart) {
    SwagSong swagSong;
    swagSong.song = chart.title;
    swagSong.bpm = chart.bpm;
    swagSong.speed = chart.speed;
    swagSong.keyCount = chart.keyCount;
    swagSong.sections = static_cast<int>(chart.sections.size());
    swagSong.validScore = true;

    for (const auto& section : chart.sections) {
        SwagSection swagSection;
        swagSection.lengthInSteps = section.lengthInSteps;
        swagSection.bpm = section.bpm;
        swagSection.changeBPM = section.changeBPM;
        swagSection.mustHitSection = true;
        swagSection.typeOfSection = 0;

        for (const auto& note : section.notes) {
            std::vector<float> noteData = {
                note.time,
                static_cast<float>(note.lane),
                0.0f
            };
            if (note.isHold) {
                noteData.push_back(note.duration);
            }
            swagSection.sectionNotes.push_back(noteData);
        }

        swagSong.notes.push_back(swagSection);
        swagSong.sectionLengths.push_back(swagSection);
    }

    return swagSong;
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
                    section.changeBPM = noteJson.value("changeBPM", false);
                    section.bpm = noteJson.value("bpm", swagShit.bpm);
                    
                    if (noteJson.contains("sectionNotes") && noteJson["sectionNotes"].is_array()) {
                        for (const auto& noteData : noteJson["sectionNotes"]) {
                            if (noteData.is_array() && noteData.size() >= 2) {
                                std::vector<float> note;
                                note.push_back(noteData[0].get<float>());
                                note.push_back(noteData[1].get<float>());
                                note.push_back(0.0f);
                                
                                if (noteData.size() >= 4) {
                                    note.push_back(noteData[3].get<float>());
                                        }
                                
                                    section.sectionNotes.push_back(note);
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