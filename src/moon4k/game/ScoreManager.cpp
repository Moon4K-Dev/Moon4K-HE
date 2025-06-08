#include "ScoreManager.h"
#include "../../engine/utils/Log.h"
#include <fstream>
#include <algorithm>
#include <filesystem>

ScoreManager* ScoreManager::instance = nullptr;

ScoreManager::ScoreManager() {
    scores = {{"scores", nlohmann::json::array()}};
    loadScores();
}

ScoreManager* ScoreManager::getInstance() {
    if (!instance) {
        instance = new ScoreManager();
    }
    return instance;
}

void ScoreManager::loadScores() {
    std::filesystem::path scoresDir = std::filesystem::path(scoresPath).parent_path();
    if (!std::filesystem::exists(scoresDir)) {
        std::filesystem::create_directories(scoresDir);
    }

    std::ifstream file(scoresPath);
    if (!file.is_open()) {
        Log::getInstance().info("No scores file found, creating new one");
        saveScores();
        return;
    }

    try {
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        if (content.empty()) {
            Log::getInstance().info("Scores file is empty, initializing new one");
            scores = {{"scores", nlohmann::json::array()}};
            return;
        }
        scores = nlohmann::json::parse(content);
        if (!scores.contains("scores") || !scores["scores"].is_array()) {
            Log::getInstance().info("Scores file has invalid format, reinitializing");
            scores = {{"scores", nlohmann::json::array()}};
        }
    } catch (const std::exception& e) {
        Log::getInstance().error("Failed to parse scores file: " + std::string(e.what()));
        scores = {{"scores", nlohmann::json::array()}};
    }
}

void ScoreManager::saveScores() {
    std::filesystem::path scoresDir = std::filesystem::path(scoresPath).parent_path();
    if (!std::filesystem::exists(scoresDir)) {
        std::filesystem::create_directories(scoresDir);
    }

    std::ofstream file(scoresPath);
    if (!file.is_open()) {
        Log::getInstance().error("Failed to open scores file for writing");
        return;
    }

    try {
        file << scores.dump(4);
    } catch (const std::exception& e) {
        Log::getInstance().error("Failed to save scores: " + std::string(e.what()));
    }
}

void ScoreManager::saveScore(const std::string& songName, const std::string& difficulty,
                           int score, int misses, float accuracy, int totalNotesHit, const std::string& rank) {
    try {
        SongScore newScore{songName, difficulty, score, misses, accuracy, totalNotesHit, rank};
        
        if (!scores.contains("scores") || !scores["scores"].is_array()) {
            scores = {{"scores", nlohmann::json::array()}};
        }
        
        bool foundBetterScore = false;
        
        for (auto& existingScore : scores["scores"]) {
            if (existingScore["songName"] == songName && existingScore["difficulty"] == difficulty) {
                if (score > existingScore["score"].get<int>()) {
                    existingScore = newScore.toJson();
                    Log::getInstance().info("Updated high score for " + songName + " (" + difficulty + ")");
                } else {
                    foundBetterScore = true;
                }
                break;
            }
        }
        
        if (!foundBetterScore) {
            scores["scores"].push_back(newScore.toJson());
            Log::getInstance().info("Saved new score for " + songName + " (" + difficulty + ")");
        }
        
        saveScores();
    } catch (const std::exception& e) {
        Log::getInstance().error("Failed to save score: " + std::string(e.what()));
    }
}

SongScore ScoreManager::getHighScore(const std::string& songName, const std::string& difficulty) {
    try {
        if (!scores.contains("scores") || !scores["scores"].is_array()) {
            return SongScore{};
        }
        
        for (const auto& score : scores["scores"]) {
            if (score["songName"] == songName && score["difficulty"] == difficulty) {
                return SongScore::fromJson(score);
            }
        }
    } catch (const std::exception& e) {
        Log::getInstance().error("Failed to get high score: " + std::string(e.what()));
    }
    
    return SongScore{};
}

std::vector<SongScore> ScoreManager::getAllScores(const std::string& songName) {
    std::vector<SongScore> songScores;
    
    try {
        if (!scores.contains("scores") || !scores["scores"].is_array()) {
            return songScores;
        }
        
        for (const auto& score : scores["scores"]) {
            if (score["songName"] == songName) {
                songScores.push_back(SongScore::fromJson(score));
            }
        }
        
        std::sort(songScores.begin(), songScores.end(), 
            [](const SongScore& a, const SongScore& b) { return a.score > b.score; });
    } catch (const std::exception& e) {
        Log::getInstance().error("Failed to get all scores: " + std::string(e.what()));
    }
    
    return songScores;
} 