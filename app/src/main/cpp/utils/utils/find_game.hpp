#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <SDL.h>

#if defined(__ANDROID__)
#include <jni.h>
#else
#include "steam/steam_api.h"
#endif

class FindGame {
public:
    explicit FindGame();
    ~FindGame();

    std::filesystem::path getGamePath();

#if !defined(__ANDROID__)
    private:
#if defined(_WIN32)
    std::string exeFile = "SENSE_THE_GAME.exe"; // Windows
#elif defined(__linux__)
    std::string exeFile = "SENSE_THE_GAME.sh";  // Linux
#endif

    std::string readFile(const std::filesystem::path& path);
    std::vector<std::filesystem::path> getSteamLibraries(const std::filesystem::path& steamRoot);

    bool initSteam();
    void ensureSteamAppIdFile();
#endif
};

