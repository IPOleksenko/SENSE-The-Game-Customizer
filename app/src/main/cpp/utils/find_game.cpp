#include <utils/find_game.hpp>

namespace fs = std::filesystem;

#if defined(__ANDROID__)
FindGame::FindGame() = default;
FindGame::~FindGame() = default;

#include <jni.h>
#include <SDL.h>
#include <filesystem>

namespace fs = std::filesystem;

fs::path FindGame::getGamePath() {
    const std::string packageName = "com.ipoleksenko.sense";

    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
    if (!env) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "JNIEnv is null!");
        return {};
    }

    jclass activityThreadClass = env->FindClass("android/app/ActivityThread");
    if (!activityThreadClass) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to find ActivityThread class");
        return {};
    }

    jmethodID currentApplicationMethod =
            env->GetStaticMethodID(activityThreadClass, "currentApplication", "()Landroid/app/Application;");
    if (!currentApplicationMethod) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to find currentApplication()");
        env->DeleteLocalRef(activityThreadClass);
        return {};
    }

    jobject appInstance = env->CallStaticObjectMethod(activityThreadClass, currentApplicationMethod);
    if (!appInstance) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get Application instance");
        env->DeleteLocalRef(activityThreadClass);
        return {};
    }

    jclass contextClass = env->GetObjectClass(appInstance);
    jmethodID getPackageManagerMethod =
            env->GetMethodID(contextClass, "getPackageManager", "()Landroid/content/pm/PackageManager;");
    if (!getPackageManagerMethod) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to find getPackageManager()");
        env->DeleteLocalRef(contextClass);
        env->DeleteLocalRef(activityThreadClass);
        return {};
    }

    jobject packageManager = env->CallObjectMethod(appInstance, getPackageManagerMethod);
    if (!packageManager) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get PackageManager");
        env->DeleteLocalRef(contextClass);
        env->DeleteLocalRef(activityThreadClass);
        return {};
    }

    jclass pmClass = env->GetObjectClass(packageManager);
    jmethodID getPackageInfoMethod =
            env->GetMethodID(pmClass, "getPackageInfo", "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;");
    if (!getPackageInfoMethod) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to find getPackageInfo()");
        env->DeleteLocalRef(pmClass);
        env->DeleteLocalRef(packageManager);
        env->DeleteLocalRef(contextClass);
        env->DeleteLocalRef(activityThreadClass);
        return {};
    }

    jstring pkgName = env->NewStringUTF(packageName.c_str());
    env->CallObjectMethod(packageManager, getPackageInfoMethod, pkgName, 0);

    bool installed = true;
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        installed = false;
    }

    env->DeleteLocalRef(pkgName);
    env->DeleteLocalRef(pmClass);
    env->DeleteLocalRef(packageManager);
    env->DeleteLocalRef(contextClass);
    env->DeleteLocalRef(activityThreadClass);

    if (!installed) {
        SDL_Log("Package %s not installed", packageName.c_str());
        return {};
    }

    jobject activity = (jobject)SDL_AndroidGetActivity();
    if (!activity) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get SDL activity");
        return {};
    }

    jclass activityClass = env->GetObjectClass(activity);
    jmethodID getExternalFilesDirMethod =
            env->GetMethodID(activityClass, "getExternalFilesDir", "(Ljava/lang/String;)Ljava/io/File;");
    if (!getExternalFilesDirMethod) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to find getExternalFilesDir()");
        env->DeleteLocalRef(activityClass);
        return {};
    }

    jobject dirObj = env->CallObjectMethod(activity, getExternalFilesDirMethod, nullptr);
    if (!dirObj) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "getExternalFilesDir() returned null");
        env->DeleteLocalRef(activityClass);
        return {};
    }

    jclass fileClass = env->FindClass("java/io/File");
    jmethodID getAbsolutePathMethod =
            env->GetMethodID(fileClass, "getAbsolutePath", "()Ljava/lang/String;");
    if (!getAbsolutePathMethod) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to find getAbsolutePath()");
        env->DeleteLocalRef(fileClass);
        env->DeleteLocalRef(dirObj);
        env->DeleteLocalRef(activityClass);
        return {};
    }

    jstring jPath = (jstring)env->CallObjectMethod(dirObj, getAbsolutePathMethod);
    const char* cPath = env->GetStringUTFChars(jPath, nullptr);
    fs::path basePath = cPath;

    env->ReleaseStringUTFChars(jPath, cPath);
    env->DeleteLocalRef(jPath);
    env->DeleteLocalRef(fileClass);
    env->DeleteLocalRef(dirObj);
    env->DeleteLocalRef(activityClass);

    std::string base = basePath.string();
    size_t pos = base.find("/Android/data/");
    if (pos == std::string::npos) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unexpected external path: %s", base.c_str());
        return {};
    }

    std::string targetPath = base.substr(0, pos) + "/Android/media/" + packageName;
    fs::path mediaPath = targetPath;

    try {
        if (!fs::exists(mediaPath)) {
            fs::create_directories(mediaPath);
            SDL_Log("Created directory: %s", mediaPath.string().c_str());
        }
    } catch (const std::exception& e) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create directory: %s", e.what());
        return {};
    }

    SDL_Log("Game media path: %s", mediaPath.string().c_str());
    return mediaPath;
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
    } else {
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
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Executable file not found in path: %s", exePath.string().c_str());
        return {};
    }

    SDL_Log("Steam game path: %s", gamePath.string().c_str());
    return gamePath;
}

#endif
