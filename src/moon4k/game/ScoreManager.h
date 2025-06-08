#pragma once

#include <string>
#include <vector>
#include "../backend/json.hpp"

struct SongScore {
    std::string songName;
    std::string difficulty;
    int score;
    int misses;
    float accuracy;
    int totalNotesHit;
    std::string rank;
    
    nlohmann::json toJson() const {
        nlohmann::json j;
        j["songName"] = songName;
        j["difficulty"] = difficulty;
        j["score"] = score;
        j["misses"] = misses;
        j["accuracy"] = accuracy;
        j["totalNotesHit"] = totalNotesHit;
        j["rank"] = rank;
        return j;
    }
    
    static SongScore fromJson(const nlohmann::json& j) {
        SongScore score;
        score.songName = j["songName"].get<std::string>();
        score.difficulty = j["difficulty"].get<std::string>();
        score.score = j["score"].get<int>();
        score.misses = j["misses"].get<int>();
        score.accuracy = j["accuracy"].get<float>();
        score.totalNotesHit = j["totalNotesHit"].get<int>();
        score.rank = j["rank"].get<std::string>();
        return score;
    }
};

class ScoreManager {
private:
    static ScoreManager* instance;
    std::string scoresPath = "assets/scores.json";
    nlohmann::json scores;

    ScoreManager();
    void loadScores();
    void saveScores();

public:
    static ScoreManager* getInstance();
    
    void saveScore(const std::string& songName, const std::string& difficulty, 
                  int score, int misses, float accuracy, int totalNotesHit, const std::string& rank);
                  
    SongScore getHighScore(const std::string& songName, const std::string& difficulty);
    std::vector<SongScore> getAllScores(const std::string& songName);
}; 