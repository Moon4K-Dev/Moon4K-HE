#pragma once

#include "../SwagState.h"
#include "../../engine/graphics/Text.h"
#include "../../engine/audio/Sound.h"
#include "../../engine/utils/Discord.h"
#include "../../engine/core/Engine.h"
#include "../../engine/input/Input.h"
#include "../../engine/core/SDLManager.h"
#include "../../engine/utils/Paths.h"
#include <vector>
#include <string>
#include <curl/curl.h>
#if !_WIN32
#include <miniz.h>
#else
#include <miniz/miniz.h>
#endif
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>

struct LoadedImageData {
    std::vector<uint8_t> data;
    size_t beatmapIndex;
};

struct BeatmapInfo {
    std::string name;
    std::string downloadUrl;
    std::string description;
    std::string author;
    std::string imageUrl;
    SDL_Texture* thumbnail;
    float x;
    float y;
    float width;
    float height;
    bool isLoading;
};

struct SidebarInfo {
    bool isVisible;
    float x;
    float targetX;
    float width;
    float alpha;
    float targetAlpha;
    size_t selectedBeatmapIndex;
    SDL_Texture* thumbnail;
    SDL_Rect thumbnailRect;
};

class OnlineDLState : public SwagState {
public:
    OnlineDLState();
    ~OnlineDLState();

    void create() override;
    void update(float deltaTime) override;
    void render() override;
    void destroy() override;

private:
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    void fetchDirectoryListing(const std::string& url);
    void parseDirectoryListing(const std::string& data);
    void updatePage();
    void prevPage();
    void nextPage();
    void showErrorMessage(const std::string& message);
    void loadThumbnail(BeatmapInfo& beatmap);
    void renderBackground();
    void handleMouseClick();
    SDL_Surface* loadImageFromMemory(const void* data, size_t size);
    
    void showSidebar(size_t beatmapIndex);
    void hideSidebar();
    void updateSidebar(float deltaTime);
    void renderSidebar();
    
    void thumbnailLoaderThread();
    void queueThumbnailLoad(size_t beatmapIndex);
    void processLoadedImages();
    std::string wrapText(const std::string& text, float maxWidth, int fontSize);
    
    void downloadBeatmap(size_t beatmapIndex);
    static size_t WriteToFile(void* ptr, size_t size, size_t nmemb, void* stream);
    void showSuccessMessage(const std::string& message);
    
    std::vector<BeatmapInfo> beatmaps;
    Text* titleText;
    Text* pageText;
    Text* errorText;

    int pageSize;
    int currentPage;
    int totalPages;
    
    SDL_Color bgColor;
    SDL_Color gridColor;
    
    float itemWidth;
    float itemHeight;
    float gridSpacing;
    
    bool isTransitioning;
    std::string jsonData;
    CURL* curl;

    std::thread loaderThread;
    std::mutex queueMutex;
    std::mutex beatmapsMutex;
    std::mutex loadedImagesMutex;
    std::queue<size_t> loadQueue;
    std::queue<LoadedImageData> loadedImages;
    std::atomic<bool> threadRunning;
    
    SidebarInfo sidebar;
    Text* sidebarTitleText;
    Text* sidebarAuthorText;
    Text* sidebarDescText;
    Text* sidebarInstructionsText;
    float sidebarTweenSpeed;
    bool isHandlingClick;

    Text* downloadProgressText;
    Text* successMessageText;
    Text* errorMessageText;
    bool isDownloading;
    double downloadProgress;
    static int ProgressCallback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow);
    void updateDownloadText();
};
