#include "UserSession.h"
#include <fstream>

std::string UserSession::currentUsername;
const char* UserSession::SESSION_FILE = "assets/session.dat";

void UserSession::Initialize() {
    LoadSession();
}

void UserSession::SetUsername(const std::string& username) {
    currentUsername = username;
    SaveSession();
}

void UserSession::Logout() {
    currentUsername.clear();
    SaveSession();
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