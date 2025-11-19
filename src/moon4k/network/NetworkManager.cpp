#include "NetworkManager.h"
#include "../../engine/utils/Log.h"

NetworkManager::NetworkManager()
    : curl(nullptr)
    , isConnected(false) {
}

NetworkManager::~NetworkManager() {
    Shutdown();
}

size_t NetworkManager::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

bool NetworkManager::Initialize() {
    curl = curl_easy_init();
    if (!curl) {
        lastError = "Failed to initialize CURL";
        Log::getInstance().error(lastError);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);

    if (!PingServer()) {
        lastError = "Failed to connect to server";
        Log::getInstance().error(lastError);
        return false;
    }

    isConnected = true;
    Log::getInstance().info("Successfully connected to server");
    return true;
}

void NetworkManager::Shutdown() {
    if (curl) {
        curl_easy_cleanup(curl);
        curl = nullptr;
    }
    isConnected = false;
}

bool NetworkManager::PingServer() {
    if (!curl) return false;

    std::string url = std::string(serverUrl) + "/ping";
    std::string response;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        lastError = curl_easy_strerror(res);
        return false;
    }

    try {
        json responseJson = json::parse(response);
        return responseJson["status"] == "ok";
    } catch (const json::parse_error&) {
        lastError = "Invalid server response";
        return false;
    }
}

bool NetworkManager::SendRequest(const std::string& endpoint, const std::string& postData, std::string& response) {
    if (!curl) return false;

    std::string url = std::string(serverUrl) + endpoint;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    if (!postData.empty()) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    } else {
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    }

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        lastError = curl_easy_strerror(res);
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200) {
        try {
            json responseJson = json::parse(response);
            if (responseJson.contains("error")) {
                lastError = responseJson["error"].get<std::string>();
            } else {
                lastError = "Server returned error code: " + std::to_string(http_code);
            }
        } catch (const json::parse_error&) {
            lastError = "Server returned error code: " + std::to_string(http_code);
        }
        return false;
    }

    return true;
} 