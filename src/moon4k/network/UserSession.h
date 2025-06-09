#pragma once
#include <string>
#include <fstream>

class UserSession {
public:
    static void Initialize();
    static void SetUsername(const std::string& username);
    static const std::string& GetUsername() { return currentUsername; }
    static bool IsLoggedIn() { return !currentUsername.empty(); }
    static void Logout();

private:
    static void SaveSession();
    static void LoadSession();
    static std::string currentUsername;
    static const char* SESSION_FILE;
}; 