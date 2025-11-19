#pragma once

#include <string>
#include <memory>
#include <curl/curl.h>
#include "../backend/json.hpp"

using json = nlohmann::json;

class NetworkManager {
public:
    static NetworkManager& GetInstance() {
        static NetworkManager instance;
        return instance;
    }

    bool Initialize();
    void Shutdown();
    bool IsConnected() const { return isConnected; }
    const std::string& GetLastError() const { return lastError; }
    
    bool PingServer();
    
    bool SendRequest(const std::string& endpoint, const std::string& postData, std::string& response);

private:
    NetworkManager();
    ~NetworkManager();
    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    
    CURL* curl;
    bool isConnected;
    std::string lastError;
    const char* serverUrl = "http://localhost:7777";
}; 