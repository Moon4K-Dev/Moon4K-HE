#include "UserSession.h"
#include <fstream>
#include <windows.h>
#include <shlobj.h>
#include <sstream>

std::string UserSession::currentUsername;
const char* UserSession::SESSION_FILE = "assets/session.dat";

std::string UserSession::GetWindowsUsername() {
    char username[256];
    DWORD username_len = 256;
    if (GetUserNameA(username, &username_len)) {
        std::string uname(username);
        size_t pos = uname.find('\\');
        if (pos != std::string::npos) {
            uname = uname.substr(pos + 1);
        }
        return uname;
    }
    return "";
}

std::string UserSession::GetAuthFilePath() {
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, path))) {
        std::string docPath(path);
        return docPath + "\\Moon4KAuth.dat";
    }
    return "Moon4KAuth.dat";
}

void UserSession::SaveAuthFile() {
    std::string winUser = GetWindowsUsername();
    std::string authPath = GetAuthFilePath();
    std::ofstream file(authPath);
    if (file.is_open()) {
        file << winUser << ":" << currentUsername;
        file.close();
    }
}

void UserSession::LoadAuthFile() {
    std::string authPath = GetAuthFilePath();
    std::ifstream file(authPath);
    if (file.is_open()) {
        std::string line;
        if (std::getline(file, line)) {
            size_t sep = line.find(":");
            if (sep != std::string::npos) {
                std::string winUser = line.substr(0, sep);
                std::string moonUser = line.substr(sep + 1);
                std::string curWinUser = GetWindowsUsername();
                if (winUser == curWinUser) {
                    currentUsername = moonUser;
                } else {
                    currentUsername.clear();
                }
            }
        }
        file.close();
    }
}

void UserSession::Initialize() {
    LoadAuthFile();
}

void UserSession::SetUsername(const std::string& username) {
    currentUsername = username;
    SaveSession();
    SaveAuthFile();
}

void UserSession::Logout() {
    currentUsername.clear();
    SaveSession();
    SaveAuthFile();
}

void UserSession::SaveSession() {
    std::ofstream file(SESSION_FILE);
    if (file.is_open()) {
        file << currentUsername;
        file.close();
    }
}

void UserSession::LoadSession() {
    std::ifstream file(SESSION_FILE);
    if (file.is_open()) {
        std::getline(file, currentUsername);
        file.close();
    }
} 