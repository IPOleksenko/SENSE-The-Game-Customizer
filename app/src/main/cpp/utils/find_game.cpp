#include <utils/find_game.hpp>

namespace fs = std::filesystem;

#if defined(__ANDROID__)
FindGame::FindGame() = default;

FindGame::~FindGame() = default;

bool FindGame::isAppInstalled() {
    const std::string packageName = "com.ipoleksenko.sense";

    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();

    if (!env) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "JNIEnv is null!");
        return false;
    }

    jclass activityThreadClass = env->FindClass("android/app/ActivityThread");
    if (!activityThreadClass) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to find ActivityThread class");
        return false;
    }

    jmethodID currentApplicationMethod = env->GetStaticMethodID(
        activityThreadClass, "currentApplication", "()Landroid/app/Application;");
    jobject appInstance = env->CallStaticObjectMethod(activityThreadClass, currentApplicationMethod);
    if (!appInstance) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get Application instance");
        return false;
    }

    jclass contextClass = env->GetObjectClass(appInstance);
    jmethodID getPackageManagerMethod = env->GetMethodID(
        contextClass, "getPackageManager", "()Landroid/content/pm/PackageManager;");
    jobject packageManager = env->CallObjectMethod(appInstance, getPackageManagerMethod);
    if (!packageManager) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get PackageManager");
        return false;
    }

    jclass pmClass = env->GetObjectClass(packageManager);
    jmethodID getPackageInfoMethod = env->GetMethodID(
        pmClass, "getPackageInfo", "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;");

    jstring pkgName = env->NewStringUTF(packageName.c_str());
    bool installed = true;
    jobject packageInfo = env->CallObjectMethod(packageManager, getPackageInfoMethod, pkgName, 0);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        installed = false;
    }

    env->DeleteLocalRef(pkgName);
    env->DeleteLocalRef(pmClass);
    env->DeleteLocalRef(packageManager);
    env->DeleteLocalRef(contextClass);
    env->DeleteLocalRef(activityThreadClass);

    return installed;
}
#else
FindGame::FindGame() {
    ensureSteamAppIdFile();
}

FindGame::~FindGame() {
    SteamAPI_Shutdown();
}

void FindGame::ensureSteamAppIdFile() {
    fs::path exeDir = fs::current_path();
    fs::path appIdFile = exeDir / "steam_appid.txt";

    std::ofstream out(appIdFile);
    if (out.is_open()) {
        out << 4051160;
        out.close();
        SDL_Log("Created steam_appid.txt with AppID: %d", 4051160);
    }
    else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create steam_appid.txt!");
    }
}

bool FindGame::initSteam() {
    if (!SteamAPI_Init()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize Steam API!");
        return false;
    }
    return true;
}

fs::path FindGame::getGamePath() {
    if (!initSteam()) {
        return {};
    }

    char pathBuffer[32767];
    AppId_t appIdNum = static_cast<AppId_t>(3832650);

    uint32 len = SteamApps()->GetAppInstallDir(appIdNum, pathBuffer, sizeof(pathBuffer));
    fs::path gamePath = fs::path(pathBuffer);

    if (len == 0 || !fs::exists(gamePath)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "The game is not installed or the folder was not found!");
        return {};
    }

    fs::path exePath = gamePath / exeFile;

    if (!fs::exists(exePath)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Executable file not found in the installation path: %s", exePath.c_str());
        return {};
    }
    return gamePath;
}
#endif