#include "DebugUI.h"
#include "../core/Engine.h"
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

DebugUI::DebugUI() {
    fpsText = new Text(10, 10, 500);
    ramText = new Text(10, 30, 500);
    memoryText = new Text(10, 50, 500);
    fpsText->setText("FPS: 0");
    ramText->setText("RAM: 0 MB");
    memoryText->setText("Memory: 0 MB");
    fpsText->setFormat("assets/fonts/5by7.ttf", 14, 0xFFFFFFFF);
    ramText->setFormat("assets/fonts/5by7.ttf", 14, 0xFFFFFFFF);
    memoryText->setFormat("assets/fonts/5by7.ttf", 14, 0xFFFFFFFF);
    
    fpsUpdateTimer = 0.0f;
    currentFPS = 0.0f;
}

DebugUI::~DebugUI() {
    delete fpsText;
    delete ramText;
    delete memoryText;
}

void DebugUI::update(float deltaTime) {
    updateFPS(deltaTime);
    updateMemoryStats();
}

void DebugUI::render() {
    fpsText->render();
    ramText->render();
    memoryText->render();
}

void DebugUI::updateFPS(float deltaTime) {
    fpsUpdateTimer += deltaTime;
    if (fpsUpdateTimer >= FPS_UPDATE_INTERVAL) {
        currentFPS = 1.0f / deltaTime;
        std::stringstream ss;
        ss << "FPS: " << std::fixed << std::setprecision(1) << currentFPS;
        fpsText->setText(ss.str());
        fpsUpdateTimer = 0.0f;
    }
}

void DebugUI::updateMemoryStats() {
#ifdef _WIN32
    updateMemoryStatsWindows();
#elif defined(__SWITCH__)
    updateMemoryStatsSwitch();
#elif defined(__linux__)
    updateMemoryStatsLinux();
#endif
}

#ifdef _WIN32
void DebugUI::updateMemoryStatsWindows() {
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        std::stringstream ramSS;
        ramSS << "RAM: " << std::fixed << std::setprecision(1) 
              << (pmc.WorkingSetSize / (1024.0f * 1024.0f)) << " MB";
        ramText->setText(ramSS.str());

        std::stringstream memSS;
        memSS << "Memory: " << std::fixed << std::setprecision(1) 
              << (pmc.PrivateUsage / (1024.0f * 1024.0f)) << " MB";
        memoryText->setText(memSS.str());
    }
}
#endif

#ifdef __SWITCH__
void DebugUI::updateMemoryStatsSwitch() {
    u64 totalMemory = 0;
    u64 freeMemory = 0;
    svcGetSystemInfo(&totalMemory, 0, 0, 0);
    svcGetSystemInfo(&freeMemory, 0, 0, 1);
    
    u64 usedMemory = totalMemory - freeMemory;
    
    std::stringstream ramSS;
    ramSS << "RAM: " << std::fixed << std::setprecision(1) 
          << (usedMemory / (1024.0f * 1024.0f)) << " MB";
    ramText->setText(ramSS.str());

    std::stringstream memSS;
    memSS << "Memory: " << std::fixed << std::setprecision(1) 
          << (totalMemory / (1024.0f * 1024.0f)) << " MB";
    memoryText->setText(memSS.str());
}
#endif

#ifdef __linux__
void DebugUI::updateMemoryStatsLinux() {
    std::ifstream status("/proc/self/status");
    std::string line;
    long vmSize = 0;
    long vmRSS = 0;

    while (std::getline(status, line)) {
        if (line.compare(0, 7, "VmSize:") == 0) {
            std::stringstream ss(line.substr(7));
            ss >> vmSize;
        }
        else if (line.compare(0, 6, "VmRSS:") == 0) {
            std::stringstream ss(line.substr(6));
            ss >> vmRSS;
        }
    }

    std::stringstream ramSS;
    ramSS << "RAM: " << std::fixed << std::setprecision(1) 
          << (vmRSS / 1024.0f) << " MB";
    ramText->setText(ramSS.str());

    std::stringstream memSS;
    memSS << "Memory: " << std::fixed << std::setprecision(1) 
          << (vmSize / 1024.0f) << " MB";
    memoryText->setText(memSS.str());
}
#endif 