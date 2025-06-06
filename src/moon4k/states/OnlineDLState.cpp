#include "OnlineDLState.h"
#include "MainMenuState.h"
#include "../backend/json.hpp"
#include <SDL2/SDL2_gfxPrimitives.h>
#include <algorithm>
#include <sstream>
#include <filesystem>
#include <miniz/miniz.h>

using json = nlohmann::json;

OnlineDLState::OnlineDLState() 
    : pageSize(12)
    , currentPage(0)
    , totalPages(0)
    , titleText(nullptr)
    , pageText(nullptr)
    , errorText(nullptr)
    , sidebarTitleText(nullptr)
    , sidebarAuthorText(nullptr)
    , sidebarDescText(nullptr)
    , sidebarInstructionsText(nullptr)
    , itemWidth(200.0f)
    , itemHeight(150.0f)
    , gridSpacing(30.0f)
    , isTransitioning(false)
    , curl(nullptr)
    , threadRunning(true)
    , sidebarTweenSpeed(8.0f)
    , isHandlingClick(false)
    , downloadProgressText(nullptr)
    , successMessageText(nullptr)
    , errorMessageText(nullptr)
    , isDownloading(false)
    , downloadProgress(0.0) {
    bgColor = {20, 20, 20, 255};
    gridColor = {255, 255, 255, 51};

    sidebar.isVisible = false;
    sidebar.width = 400.0f;
    sidebar.x = Engine::getInstance()->getWindowWidth();
    sidebar.targetX = sidebar.x;
    sidebar.alpha = 0.0f;
    sidebar.targetAlpha = 0.0f;
    sidebar.thumbnail = nullptr;
    sidebar.selectedBeatmapIndex = 0;
    loaderThread = std::thread(&OnlineDLState::thumbnailLoaderThread, this);
}

OnlineDLState::~OnlineDLState() {
    threadRunning = false;
    
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        std::queue<size_t> empty;
        std::swap(loadQueue, empty);
    }
    
    if (loaderThread.joinable()) {
        loaderThread.join();
    }

    destroy();
    if (curl) {
        curl_easy_cleanup(curl);
        curl = nullptr;
    }
    IMG_Quit();
}

SDL_Surface* OnlineDLState::loadImageFromMemory(const void* data, size_t size) {
    SDL_RWops* rw = SDL_RWFromMem(const_cast<void*>(data), static_cast<int>(size));
    if (!rw) {
        Log::getInstance().error("Failed to create RWops from memory: " + std::string(SDL_GetError()));
        return nullptr;
    }

    SDL_Surface* surface = IMG_Load_RW(rw, 1);
    if (!surface) {
        Log::getInstance().error("Failed to load image from memory: " + std::string(IMG_GetError()));
        return nullptr;
    }

    return surface;
}

size_t OnlineDLState::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    std::string* str = static_cast<std::string*>(userp);
    str->append((char*)contents, realsize);
    return realsize;
}

void OnlineDLState::create() {
    SwagState::create();
    
    Engine* engine = Engine::getInstance();
    
    titleText = new Text();
    titleText->setFormat(Paths::font("vcr.ttf"), 32, 0xFFFFFFFF);
    titleText->setText("Online Maps");
    titleText->setPosition(20, 20);
    engine->addText(titleText);
    
    pageText = new Text();
    pageText->setFormat(Paths::font("vcr.ttf"), 16, 0xFFFFFFFF);
    pageText->setText("Page 1 / 1");
    pageText->setPosition(static_cast<float>(engine->getWindowWidth()) / 2.0f - 50.0f, 
                         static_cast<float>(engine->getWindowHeight() - 40));
    engine->addText(pageText);

    curl = curl_easy_init();
    if (curl) {
        fetchDirectoryListing("https://raw.githubusercontent.com/yophlox/Moon4K-OnlineMaps/refs/heads/main/maps.json");
    } else {
        showErrorMessage("Failed to initialize CURL");
    }

    Discord::GetInstance().SetDetails("Browsing Online Maps...");
    Discord::GetInstance().Update();
}

void OnlineDLState::fetchDirectoryListing(const std::string& url) {
    if (!curl) return;

    jsonData.clear();
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &jsonData);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::string error = "Failed to fetch map list: " + std::string(curl_easy_strerror(res));
        Log::getInstance().error(error);
        showErrorMessage(error);
    } else {
        parseDirectoryListing(jsonData);
    }
}

void OnlineDLState::parseDirectoryListing(const std::string& data) {
    try {
        if (data.empty()) {
            Log::getInstance().error("Directory listing data is empty!");
            showErrorMessage("No beatmap data received");
            return;
        }

        json j = json::parse(data);
        if (!j.is_array()) {
            Log::getInstance().error("JSON data is not an array!");
            showErrorMessage("Invalid beatmap data format");
            return;
        }

        beatmaps.clear();

        for (const auto& map : j) {
            if (!map.contains("name") || !map.contains("download") || 
                !map.contains("desc") || !map.contains("author") || 
                !map.contains("image")) {
                continue;
            }

            BeatmapInfo info;
            info.name = map["name"].get<std::string>();
            info.downloadUrl = map["download"].get<std::string>();
            info.description = map["desc"].get<std::string>();
            info.author = map["author"].get<std::string>();
            info.imageUrl = map["image"].get<std::string>();
            info.thumbnail = nullptr;
            info.isLoading = false;
            beatmaps.push_back(info);            
            Log::getInstance().info("Added beatmap: " + info.name + " with image: " + info.imageUrl);
        }

        totalPages = static_cast<int>((beatmaps.size() + pageSize - 1) / pageSize);
        
        size_t startIndex = 0;
        size_t endIndex = std::clamp(startIndex + pageSize, size_t(0), beatmaps.size());
        
        for (size_t i = startIndex; i < endIndex; i++) {
            queueThumbnailLoad(i);
        }
        
        updatePage();

    } catch (const std::exception& e) {
        Log::getInstance().error("Failed to parse map list: " + std::string(e.what()));
        showErrorMessage("Failed to parse map list: " + std::string(e.what()));
    }
}

void OnlineDLState::thumbnailLoaderThread() {
    while (threadRunning) {
        size_t index = 0;
        bool hasItem = false;
        
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (!loadQueue.empty()) {
                index = loadQueue.front();
                loadQueue.pop();
                hasItem = true;
            }
        }
        
        if (hasItem) {
            std::string imageData;
            CURL* imageCurl = curl_easy_init();
            if (imageCurl) {
                std::string url;
                {
                    std::lock_guard<std::mutex> lock(beatmapsMutex);
                    if (index >= beatmaps.size()) {
                        Log::getInstance().error("Invalid beatmap index: " + std::to_string(index));
                        continue;
                    }
                    url = beatmaps[index].imageUrl;
                }
                
                curl_easy_setopt(imageCurl, CURLOPT_URL, url.c_str());
                curl_easy_setopt(imageCurl, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(imageCurl, CURLOPT_WRITEDATA, &imageData);
                curl_easy_setopt(imageCurl, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(imageCurl, CURLOPT_SSL_VERIFYPEER, 0L);

                CURLcode res = curl_easy_perform(imageCurl);
                curl_easy_cleanup(imageCurl);

                if (res == CURLE_OK) {
                    LoadedImageData loadedData;
                    loadedData.data = std::vector<uint8_t>(imageData.begin(), imageData.end());
                    loadedData.beatmapIndex = index;
                    
                    std::lock_guard<std::mutex> lock(loadedImagesMutex);
                    loadedImages.push(std::move(loadedData));
                } else {
                    Log::getInstance().error("Failed to download image: " + std::string(curl_easy_strerror(res)));
                }
            } else {
                Log::getInstance().error("Failed to initialize CURL for image download");
            }
        } else {
            SDL_Delay(10);
        }
    }
}

void OnlineDLState::processLoadedImages() {
    LoadedImageData loadedData;
    bool hasImage = false;
    
    {
        std::lock_guard<std::mutex> lock(loadedImagesMutex);
        if (!loadedImages.empty()) {
            loadedData = std::move(loadedImages.front());
            loadedImages.pop();
            hasImage = true;
        }
    }
    
    if (hasImage) {
        SDL_Surface* surface = loadImageFromMemory(loadedData.data.data(), loadedData.data.size());
        if (surface) {
            float scaleX = itemWidth / static_cast<float>(surface->w);
            float scaleY = (itemHeight - 40) / static_cast<float>(surface->h);
            float scale = std::clamp(scaleX < scaleY ? scaleX : scaleY, 0.1f, 10.0f);

            int newWidth = static_cast<int>(surface->w * scale);
            int newHeight = static_cast<int>(surface->h * scale);

            SDL_Surface* scaledSurface = SDL_CreateRGBSurface(0, newWidth, newHeight, 32,
                0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

            if (scaledSurface) {
                SDL_BlitScaled(surface, NULL, scaledSurface, NULL);
                
                std::lock_guard<std::mutex> lock(beatmapsMutex);
                if (loadedData.beatmapIndex < beatmaps.size()) {
                    SDL_Texture* texture = SDL_CreateTextureFromSurface(SDLManager::getInstance().getRenderer(), scaledSurface);
                    if (texture) {
                        beatmaps[loadedData.beatmapIndex].thumbnail = texture;
                        beatmaps[loadedData.beatmapIndex].isLoading = false;
                    } else {
                        Log::getInstance().error("Failed to create texture: " + std::string(SDL_GetError()));
                    }
                } else {
                    Log::getInstance().error("Invalid beatmap index during texture creation: " + std::to_string(loadedData.beatmapIndex));
                }
                
                SDL_FreeSurface(scaledSurface);
            } else {
                Log::getInstance().error("Failed to create scaled surface: " + std::string(SDL_GetError()));
            }
            SDL_FreeSurface(surface);
        } else {
            Log::getInstance().error("Failed to create surface from image data");
            std::lock_guard<std::mutex> lock(beatmapsMutex);
            if (loadedData.beatmapIndex < beatmaps.size()) {
                SDL_Surface* placeholder = SDL_CreateRGBSurface(0, static_cast<int>(itemWidth),
                    static_cast<int>(itemHeight - 40), 32, 0, 0, 0, 0);
                if (placeholder) {
                    SDL_FillRect(placeholder, NULL, SDL_MapRGB(placeholder->format, 100, 100, 100));
                    SDL_Texture* texture = SDL_CreateTextureFromSurface(SDLManager::getInstance().getRenderer(), placeholder);
                    if (texture) {
                        beatmaps[loadedData.beatmapIndex].thumbnail = texture;
                    } else {
                        Log::getInstance().error("Failed to create placeholder texture: " + std::string(SDL_GetError()));
                    }
                    SDL_FreeSurface(placeholder);
                } else {
                    Log::getInstance().error("Failed to create placeholder surface: " + std::string(SDL_GetError()));
                }
                beatmaps[loadedData.beatmapIndex].isLoading = false;
            }
        }
    }
}

void OnlineDLState::updatePage() {
    Engine* engine = Engine::getInstance();
    float startX = 20.0f;
    float startY = 80.0f;
    float windowWidth = static_cast<float>(engine->getWindowWidth());
    
    size_t startIndex = currentPage * pageSize;
    size_t endIndex = std::clamp(startIndex + pageSize, size_t(0), beatmaps.size());

    std::lock_guard<std::mutex> lock(beatmapsMutex);
    for (size_t i = startIndex; i < endIndex; i++) {
        size_t gridPos = i - startIndex;
        float x = startX + (gridPos % 3) * (itemWidth + 10);
        float y = startY + (gridPos / 3) * (itemHeight + 10);
        
        beatmaps[i].x = x;
        beatmaps[i].y = y;
        beatmaps[i].width = itemWidth;
        beatmaps[i].height = itemHeight;

        if (!beatmaps[i].thumbnail && !beatmaps[i].isLoading) {
            beatmaps[i].isLoading = true;
            queueThumbnailLoad(i);
        }
    }

    std::stringstream ss;
    ss << "Page " << (currentPage + 1) << " / " << totalPages;
    pageText->setText(ss.str());
}

void OnlineDLState::queueThumbnailLoad(size_t beatmapIndex) {
    std::lock_guard<std::mutex> lock(queueMutex);
    loadQueue.push(beatmapIndex);
}

int OnlineDLState::ProgressCallback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
    OnlineDLState* state = static_cast<OnlineDLState*>(clientp);
    if (state && dltotal > 0) {
        state->downloadProgress = (dlnow / dltotal) * 100.0;
        state->updateDownloadText();
    }
    return 0;
}

void OnlineDLState::updateDownloadText() {
    if (!downloadProgressText) {
        downloadProgressText = new Text();
        downloadProgressText->setFormat(Paths::font("vcr.ttf"), 16, 0xFFFFFFFF);
        Engine::getInstance()->addText(downloadProgressText);
    }

    std::stringstream ss;
    ss << "Downloading... " << static_cast<int>(downloadProgress) << "%";
    downloadProgressText->setText(ss.str());
    downloadProgressText->setPosition(
        static_cast<float>(Engine::getInstance()->getWindowWidth()) / 2.0f - downloadProgressText->getWidth() / 2.0f,
        static_cast<float>(Engine::getInstance()->getWindowHeight()) / 2.0f
    );
}

void OnlineDLState::downloadBeatmap(size_t beatmapIndex) {
    if (beatmapIndex >= beatmaps.size() || isDownloading) return;

    const BeatmapInfo& beatmap = beatmaps[beatmapIndex];
    std::string downloadUrl = beatmap.downloadUrl;
    std::string fileName = beatmap.name;

    if (!std::filesystem::exists("assets/downloads/")) {
        std::filesystem::create_directory("assets/downloads/");
    }
    if (!std::filesystem::exists("assets/charts/")) {
        std::filesystem::create_directory("assets/charts/");
    }

    std::string tempPath = "assets/downloads/" + fileName + ".zip";
    
    CURL* dlCurl = curl_easy_init();
    if (!dlCurl) {
        showErrorMessage("Failed to initialize download");
        return;
    }

    FILE* fp = nullptr;
    errno_t err = fopen_s(&fp, tempPath.c_str(), "wb");
    if (err != 0 || !fp) {
        curl_easy_cleanup(dlCurl);
        showErrorMessage("Failed to create temporary file");
        return;
    }

    isDownloading = true;
    downloadProgress = 0.0;
    updateDownloadText();

    curl_easy_setopt(dlCurl, CURLOPT_URL, downloadUrl.c_str());
    curl_easy_setopt(dlCurl, CURLOPT_WRITEFUNCTION, WriteToFile);
    curl_easy_setopt(dlCurl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(dlCurl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(dlCurl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(dlCurl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(dlCurl, CURLOPT_PROGRESSFUNCTION, ProgressCallback);
    curl_easy_setopt(dlCurl, CURLOPT_PROGRESSDATA, this);

    CURLcode res = curl_easy_perform(dlCurl);
    curl_easy_cleanup(dlCurl);
    fclose(fp);

    if (downloadProgressText) {
        downloadProgressText = nullptr;
    }
    isDownloading = false;

    if (res != CURLE_OK) {
        showErrorMessage("Failed to download file: " + std::string(curl_easy_strerror(res)));
        std::filesystem::remove(tempPath);
        return;
    }

    Text* extractingText = new Text();
    extractingText->setFormat(Paths::font("vcr.ttf"), 16, 0xFFFFFFFF);
    extractingText->setText("Extracting files...");
    extractingText->setPosition(
        static_cast<float>(Engine::getInstance()->getWindowWidth()) / 2.0f - extractingText->getWidth() / 2.0f,
        static_cast<float>(Engine::getInstance()->getWindowHeight()) / 2.0f
    );
    Engine::getInstance()->addText(extractingText);

    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));

    if (!mz_zip_reader_init_file(&zip_archive, tempPath.c_str(), 0)) {
        showErrorMessage("Failed to open ZIP file");
        std::filesystem::remove(tempPath);
        return;
    }

    int numFiles = (int)mz_zip_reader_get_num_files(&zip_archive);
    std::string extractPath = "assets/charts/";

    bool extractSuccess = true;
    for (int i = 0; i < numFiles; i++) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) {
            extractSuccess = false;
            break;
        }

        if (mz_zip_reader_is_file_a_directory(&zip_archive, i) ||
            strstr(file_stat.m_filename, "__MACOSX") ||
            file_stat.m_filename[0] == '.') {
            continue;
        }

        std::string outPath = extractPath + file_stat.m_filename;
        std::string outDir = outPath.substr(0, outPath.find_last_of("/\\"));

        if (!outDir.empty() && !std::filesystem::exists(outDir)) {
            std::filesystem::create_directories(outDir);
        }

        if (!mz_zip_reader_extract_to_file(&zip_archive, i, outPath.c_str(), 0)) {
            extractSuccess = false;
            break;
        }
    }

    mz_zip_reader_end(&zip_archive);
    std::filesystem::remove(tempPath);
    
    if (!extractSuccess) {
        showErrorMessage("Failed to extract some files");
    } else {
        successMessageText = new Text();
        successMessageText->setFormat(Paths::font("vcr.ttf"), 24, 0x00FF00FF);
        successMessageText->setText("Chart downloaded and ready to play!");
        successMessageText->setPosition(
            static_cast<float>(Engine::getInstance()->getWindowWidth()) / 2.0f - successMessageText->getWidth() / 2.0f,
            static_cast<float>(Engine::getInstance()->getWindowHeight()) / 2.0f - 100.0f
        );
        Engine::getInstance()->addText(successMessageText);
        
        SDL_AddTimer(1500, [](Uint32 interval, void* param) -> Uint32 {
            OnlineDLState* state = static_cast<OnlineDLState*>(param);
            if (state) {
                state->hideSidebar();
            }
            return 0;
        }, this);
        
        SDL_AddTimer(3000, [](Uint32 interval, void* param) -> Uint32 {
            OnlineDLState* state = static_cast<OnlineDLState*>(param);
            if (state && state->successMessageText) {
                state->successMessageText = nullptr;
            }
            return 0;
        }, this);
    }
}

size_t OnlineDLState::WriteToFile(void* ptr, size_t size, size_t nmemb, void* stream) {
    size_t written = fwrite(ptr, size, nmemb, (FILE*)stream);
    return written;
}

void OnlineDLState::showSuccessMessage(const std::string& message) {    
    Text* successText = new Text();
    successText->setFormat(Paths::font("vcr.ttf"), 24, 0x00FF00FF);
    successText->setText(message);
    
    float xPos = static_cast<float>(Engine::getInstance()->getWindowWidth()) / 2.0f - successText->getWidth() / 2.0f;
    float yPos = static_cast<float>(Engine::getInstance()->getWindowHeight()) / 2.0f;    
    successText->setPosition(xPos, yPos);
    Engine::getInstance()->addText(successText);

    SDL_AddTimer(3000, [](Uint32 interval, void* param) -> Uint32 {
        OnlineDLState* state = static_cast<OnlineDLState*>(param);
        if (state && state->successMessageText) {
            state->successMessageText = nullptr;
        }
        return 0;
    }, this);
}

void OnlineDLState::showErrorMessage(const std::string& message) {
    Log::getInstance().error("Showing error message: " + message);
    
    errorMessageText = new Text();
    errorMessageText->setFormat(Paths::font("vcr.ttf"), 24, 0xFF0000FF);
    errorMessageText->setText(message);
    errorMessageText->setPosition(
        static_cast<float>(Engine::getInstance()->getWindowWidth()) / 2.0f - errorMessageText->getWidth() / 2.0f,
        static_cast<float>(Engine::getInstance()->getWindowHeight()) / 2.0f - 100.0f
    );
    Engine::getInstance()->addText(errorMessageText);

    SDL_AddTimer(3000, [](Uint32 interval, void* param) -> Uint32 {
        OnlineDLState* state = static_cast<OnlineDLState*>(param);
        if (state && state->errorMessageText) {
            state->errorMessageText = nullptr;
        }
        return 0;
    }, this);
}

void OnlineDLState::update(float deltaTime) {
    SwagState::update(deltaTime);
    Input::UpdateKeyStates();
    Input::UpdateControllerStates();

    if (!(SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_LEFT))) {
        isHandlingClick = false;
    }

    processLoadedImages();
    updateSidebar(deltaTime);

    if (!isTransitioning) {
        if (Input::justPressed(SDL_SCANCODE_LEFT)) {
            prevPage();
        }
        if (Input::justPressed(SDL_SCANCODE_RIGHT)) {
            nextPage();
        }
        if (Input::justPressed(SDL_SCANCODE_ESCAPE) || Input::justPressed(SDL_SCANCODE_BACKSPACE)) {
            if (sidebar.isVisible) {
                hideSidebar();
            } else {
                startTransitionOut(0.5f);
                Engine::getInstance()->switchState(new MainMenuState());
            }
        }

        if (Input::justPressed(SDL_SCANCODE_RETURN) && sidebar.isVisible) {
            downloadBeatmap(sidebar.selectedBeatmapIndex);
        }

        handleMouseClick();
    }
}

void OnlineDLState::handleMouseClick() {
    int mouseX = 0, mouseY = 0;
    Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);
    
    if (!(mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) || isHandlingClick) {
        return;
    }

    isHandlingClick = true;

    if (sidebar.isVisible && mouseX >= sidebar.x) {
        return;
    }

    size_t startIndex = currentPage * pageSize;
    size_t endIndex = std::clamp(startIndex + pageSize, size_t(0), beatmaps.size());

    bool clickedBeatmap = false;
    for (size_t i = startIndex; i < endIndex; i++) {
        const BeatmapInfo& beatmap = beatmaps[i];
        SDL_Rect bounds = {
            static_cast<int>(beatmap.x),
            static_cast<int>(beatmap.y),
            static_cast<int>(beatmap.width),
            static_cast<int>(beatmap.height)
        };

        if (mouseX >= bounds.x && mouseX < bounds.x + bounds.w &&
            mouseY >= bounds.y && mouseY < bounds.y + bounds.h) {
            showSidebar(i);
            clickedBeatmap = true;
            break;
        }
    }

    if (!clickedBeatmap && mouseX < sidebar.x) {
        hideSidebar();
    }
}

void OnlineDLState::render() {
    SDL_Renderer* renderer = SDLManager::getInstance().getRenderer();
    
    SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
    SDL_RenderClear(renderer);
    
    renderBackground();
    
    {
        std::lock_guard<std::mutex> lock(beatmapsMutex);
        size_t startIndex = currentPage * pageSize;
        size_t endIndex = std::clamp(startIndex + pageSize, size_t(0), beatmaps.size());
        
        for (size_t i = startIndex; i < endIndex; i++) {
            const BeatmapInfo& beatmap = beatmaps[i];
            
            SDL_Rect rect = {
                static_cast<int>(beatmap.x),
                static_cast<int>(beatmap.y),
                static_cast<int>(beatmap.width),
                static_cast<int>(beatmap.height)
            };
            
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
            SDL_RenderFillRect(renderer, &rect);
            
            if (beatmap.thumbnail) {
                SDL_RenderCopy(renderer, beatmap.thumbnail, nullptr, &rect);
            }
        }
    }
    
    if (titleText) titleText->render();
    if (pageText) pageText->render();
    if (errorText) errorText->render();
    if (downloadProgressText) downloadProgressText->render();
    if (successMessageText) successMessageText->render();
    if (errorMessageText) errorMessageText->render();
    
    renderSidebar();
    
    SwagState::render();
}

void OnlineDLState::renderBackground() {
    SDL_Renderer* renderer = SDLManager::getInstance().getRenderer();
    Engine* engine = Engine::getInstance();
    int windowWidth = engine->getWindowWidth();
    int windowHeight = engine->getWindowHeight();
    
    for (int x = 0; x < windowWidth; x += static_cast<int>(gridSpacing)) {
        for (int y = 0; y < windowHeight; y += static_cast<int>(gridSpacing)) {
            SDL_SetRenderDrawColor(renderer, gridColor.r, gridColor.g, gridColor.b, 
                static_cast<Uint8>(gridColor.a * (0.2f + 0.1f * sin(static_cast<float>(x + y + SDL_GetTicks()) * 0.01f))));
            SDL_RenderDrawPoint(renderer, x, y);
        }
    }
}

void OnlineDLState::prevPage() {
    if (currentPage > 0) {
        currentPage--;
        updatePage();
    }
}

void OnlineDLState::nextPage() {
    if (currentPage < totalPages - 1) {
        currentPage++;
        updatePage();
    }
}

void OnlineDLState::showSidebar(size_t beatmapIndex) {
    if (beatmapIndex >= beatmaps.size()) return;

    sidebar.x = Engine::getInstance()->getWindowWidth();
    sidebar.alpha = 0.0f;
    sidebar.selectedBeatmapIndex = beatmapIndex;
    sidebar.isVisible = true;
    sidebar.targetX = Engine::getInstance()->getWindowWidth() - sidebar.width;
    sidebar.targetAlpha = 255.0f;

    const BeatmapInfo& beatmap = beatmaps[beatmapIndex];

    Engine* engine = Engine::getInstance();

    float yPos = 50.0f;

    sidebarTitleText = new Text();
    sidebarTitleText->setFormat(Paths::font("vcr.ttf"), 24, 0xFFFFFFFF);
    std::string wrappedTitle = wrapText(beatmap.name, sidebar.width - 60, 24);
    sidebarTitleText->setText(wrappedTitle);
    sidebarTitleText->setPosition(sidebar.x + 20, yPos);
    engine->addText(sidebarTitleText);
    
    int titleLines = std::count(wrappedTitle.begin(), wrappedTitle.end(), '\n') + 1;
    yPos += titleLines * 35.0f + 10.0f;
    yPos += 10.0f;

    sidebarAuthorText = new Text();
    sidebarAuthorText->setFormat(Paths::font("vcr.ttf"), 18, 0xFFFFFFFF);
    std::string wrappedAuthor = wrapText("Author: " + beatmap.author, sidebar.width - 60, 18);
    sidebarAuthorText->setText(wrappedAuthor);
    sidebarAuthorText->setPosition(sidebar.x + 20, yPos);
    engine->addText(sidebarAuthorText);
    
    int authorLines = std::count(wrappedAuthor.begin(), wrappedAuthor.end(), '\n') + 1;
    yPos += authorLines * 28.0f + 5.0f;
    yPos += 5.0f;

    sidebarDescText = new Text();
    sidebarDescText->setFormat(Paths::font("vcr.ttf"), 16, 0xFFFFFFFF);
    std::string wrappedDesc = wrapText("Description: " + beatmap.description, sidebar.width - 60, 16);
    sidebarDescText->setText(wrappedDesc);
    sidebarDescText->setPosition(sidebar.x + 20, yPos);
    engine->addText(sidebarDescText);

    int descLines = std::count(wrappedDesc.begin(), wrappedDesc.end(), '\n') + 1;
    yPos += descLines * 25.0f + 30.0f;

    sidebar.thumbnailRect = {
        static_cast<int>(sidebar.x + 20),
        static_cast<int>(yPos),
        static_cast<int>(sidebar.width - 40),
        static_cast<int>(300)
    };

    sidebarInstructionsText = new Text();
    sidebarInstructionsText->setFormat(Paths::font("vcr.ttf"), 16, 0xFFFFFFFF);
    sidebarInstructionsText->setText("Press Enter to download\nPress ESC to close");
    sidebarInstructionsText->setPosition(sidebar.x + 20, engine->getWindowHeight() - 80);
    engine->addText(sidebarInstructionsText);

    if (beatmap.thumbnail) {
        if (sidebar.thumbnail) {
            SDL_DestroyTexture(sidebar.thumbnail);
        }
        int w, h;
        SDL_QueryTexture(beatmap.thumbnail, nullptr, nullptr, &w, &h);
        sidebar.thumbnail = SDL_CreateTexture(SDLManager::getInstance().getRenderer(),
            SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
        if (sidebar.thumbnail) {
            SDL_SetTextureBlendMode(sidebar.thumbnail, SDL_BLENDMODE_BLEND);
            SDL_SetRenderTarget(SDLManager::getInstance().getRenderer(), sidebar.thumbnail);
            SDL_RenderCopy(SDLManager::getInstance().getRenderer(), beatmap.thumbnail, nullptr, nullptr);
            SDL_SetRenderTarget(SDLManager::getInstance().getRenderer(), nullptr);
        }
    }
}

std::string OnlineDLState::wrapText(const std::string& text, float maxWidth, int fontSize) {
    std::istringstream words(text);
    std::string word;
    std::string line;
    std::string result;
    float lineWidth = 0;
    float spaceWidth = fontSize * 0.5f;
    
    while (words >> word) {
        float wordWidth = word.length() * fontSize * 0.7f;
        
        if (lineWidth + wordWidth <= maxWidth) {
            if (!line.empty()) {
                line += " ";
                lineWidth += spaceWidth;
            }
            line += word;
            lineWidth += wordWidth;
        } else {
            if (!result.empty()) {
                result += "\n";
            }
            result += line;
            line = word;
            lineWidth = wordWidth;
        }
    }
    
    if (!line.empty()) {
        if (!result.empty()) {
            result += "\n";
        }
        result += line;
    }
    
    return result;
}

void OnlineDLState::hideSidebar() {
    sidebar.isVisible = false;
    sidebar.targetX = Engine::getInstance()->getWindowWidth();
    sidebar.targetAlpha = 0.0f;
}

void OnlineDLState::updateSidebar(float deltaTime) {
    if (!sidebar.isVisible && sidebar.alpha <= 0.0f) return;

    float xDiff = sidebar.targetX - sidebar.x;
    float alphaDiff = sidebar.targetAlpha - sidebar.alpha;

    sidebar.x += xDiff * sidebarTweenSpeed * deltaTime;
    sidebar.alpha += alphaDiff * sidebarTweenSpeed * deltaTime;

    float yPos = 50.0f;

    if (sidebarTitleText) {
        sidebarTitleText->setPosition(sidebar.x + 20, yPos);
        sidebarTitleText->setAlpha(static_cast<Uint8>(sidebar.alpha));
        
        std::string text = sidebarTitleText->getText();
        int titleLines = std::count(text.begin(), text.end(), '\n') + 1;
        yPos += titleLines * 35.0f + 10.0f;
    }

    yPos += 10.0f;

    if (sidebarAuthorText) {
        sidebarAuthorText->setPosition(sidebar.x + 20, yPos);
        sidebarAuthorText->setAlpha(static_cast<Uint8>(sidebar.alpha));
        
        std::string text = sidebarAuthorText->getText();
        int authorLines = std::count(text.begin(), text.end(), '\n') + 1;
        yPos += authorLines * 28.0f + 5.0f;
    }

    yPos += 5.0f;

    if (sidebarDescText) {
        sidebarDescText->setPosition(sidebar.x + 20, yPos);
        sidebarDescText->setAlpha(static_cast<Uint8>(sidebar.alpha));
    }

    if (sidebarInstructionsText) {
        sidebarInstructionsText->setPosition(sidebar.x + 20, Engine::getInstance()->getWindowHeight() - 80);
        sidebarInstructionsText->setAlpha(static_cast<Uint8>(sidebar.alpha));
    }

    sidebar.thumbnailRect.x = static_cast<int>(sidebar.x + 20);
}

void OnlineDLState::renderSidebar() {
    if (!sidebar.isVisible && sidebar.alpha <= 0.0f) return;

    SDL_Renderer* renderer = SDLManager::getInstance().getRenderer();

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, static_cast<Uint8>(sidebar.alpha * 0.9f));
    SDL_Rect sidebarRect = {
        static_cast<int>(sidebar.x),
        0,
        static_cast<int>(sidebar.width),
        Engine::getInstance()->getWindowHeight()
    };
    SDL_RenderFillRect(renderer, &sidebarRect);

    if (sidebar.thumbnail) {
        SDL_SetTextureAlphaMod(sidebar.thumbnail, static_cast<Uint8>(sidebar.alpha));
        SDL_RenderCopy(renderer, sidebar.thumbnail, nullptr, &sidebar.thumbnailRect);
    }

    if (sidebarTitleText) sidebarTitleText->render();
    if (sidebarAuthorText) sidebarAuthorText->render();
    if (sidebarDescText) sidebarDescText->render();
    if (sidebarInstructionsText) sidebarInstructionsText->render();
}

void OnlineDLState::destroy() {
    if (sidebar.thumbnail) {
        SDL_DestroyTexture(sidebar.thumbnail);
        sidebar.thumbnail = nullptr;
    }
    
    titleText = nullptr;
    pageText = nullptr;
    errorText = nullptr;
    sidebarTitleText = nullptr;
    sidebarAuthorText = nullptr;
    sidebarDescText = nullptr;
    sidebarInstructionsText = nullptr;
    downloadProgressText = nullptr;
    successMessageText = nullptr;
    errorMessageText = nullptr;

    for (auto& beatmap : beatmaps) {
        if (beatmap.thumbnail) {
            SDL_DestroyTexture(beatmap.thumbnail);
            beatmap.thumbnail = nullptr;
        }
    }
    beatmaps.clear();
}