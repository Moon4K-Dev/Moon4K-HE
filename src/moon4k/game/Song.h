#pragma once
#include <string>
#include <vector>
#include "Section.h"

struct SwagSong {
    std::string song;
    std::vector<SwagSection> notes;
    float bpm;
    int sections;
    std::vector<SwagSection> sectionLengths;
    float speed;
    int keyCount;
    std::vector<int> timescale;
    bool validScore = false;
};

class Song {
public:
    std::string song;
    std::vector<SwagSection> notes;
    float bpm;
    int sections;
    std::vector<SwagSection> sectionLengths;
    float speed;
    int keyCount;
    std::vector<int> timescale;

    Song(const std::string& song, const std::vector<SwagSection>& notes, float bpm, int sections, int keyCount, const std::vector<int>& timescale = std::vector<int>());

    static SwagSong loadFromJson(const std::string& jsonInput, const std::string& folder = "");
    static SwagSong parseJSONshit(const std::string& rawJson);
}; 