#pragma once
#include <string>
#include <vector>
#include <memory>
#include "Tsukiyo/Chart.hpp"
#include "Tsukiyo/ChartLoader.hpp"
#include "Tsukiyo/ChartConverter.hpp"

struct SwagSection {
    int lengthInSteps;
    bool mustHitSection;
    int typeOfSection;
    bool changeBPM;
    float bpm;
    std::vector<std::vector<float>> sectionNotes;

    SwagSection() 
        : lengthInSteps(16)
        , mustHitSection(true)
        , typeOfSection(0)
        , changeBPM(false)
        , bpm(0.0f) {}
};

struct SwagSong {
    std::string song;
    float bpm;
    float speed;
    int sections;
    int keyCount;
    bool validScore;
    std::vector<SwagSection> notes;
    std::vector<SwagSection> sectionLengths;
    std::vector<int> timescale;

    SwagSong() 
        : song("")
        , bpm(100.0f)
        , speed(1.0f)
        , sections(0)
        , keyCount(4)
        , validScore(false) {}
};

class Song {
public:
    Song(const std::string& song, const std::vector<SwagSection>& notes, float bpm, int sections, int keyCount, const std::vector<int>& timescale);
    static SwagSong loadFromJson(const std::string& songName, const std::string& folder = "", const std::string& difficulty = "");
    static SwagSong parseJSONshit(const std::string& rawJson);

private:
    std::string song;
    std::vector<SwagSection> notes;
    float bpm;
    int sections;
    int keyCount;
    std::vector<int> timescale;
    float speed;
    std::vector<SwagSection> sectionLengths;

    static SwagSong convertTsukiyoToSwagSong(const Tsukiyo::Chart& chart);
    static std::string getFileExtension(const std::string& path);
}; 