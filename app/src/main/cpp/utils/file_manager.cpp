#include <utils/file_manager.hpp>
#include <assets/data.hpp>
#include <SDL.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <fstream>
#include <sstream>
#include <algorithm>
#ifdef __ANDROID__
#include <jni.h>
#endif

#if defined(__ANDROID__)
bool callFdExistsJNI(const std::string& path) {
    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
    jobject activity = (jobject)SDL_AndroidGetActivity();
    if (!env || !activity) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "JNI environment or activity is null");
        return false;
    }

    jclass activityClass = env->GetObjectClass(activity);
    if (!activityClass) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get activity class");
        return false;
    }

    jclass fileManagerClass = env->FindClass("com/ipoleksenko/sense/customizer/FileManager");
    if (!fileManagerClass) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "FileManager class not found");
        return false;
    }

    jmethodID fdExistsMethod = env->GetStaticMethodID(
            fileManagerClass,
            "fdExists",
            "(Landroid/content/Context;Ljava/lang/String;)Z"
    );

    if (!fdExistsMethod) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "fdExists method not found");
        return false;
    }

    jstring jPath = env->NewStringUTF(path.c_str());
    jboolean result = env->CallStaticBooleanMethod(fileManagerClass, fdExistsMethod, activity, jPath);

    env->DeleteLocalRef(jPath);
    env->DeleteLocalRef(fileManagerClass);
    env->DeleteLocalRef(activityClass);

    return (bool)result;
}
#endif

namespace FileManager {
    std::filesystem::path gamePath;

    // Constants
    const char* LOCALIZATION_FILE = "localization.cfg";
    const char* FONT_FILE = "font.cfg";
    const char* DECOR_CFG = "decor.cfg";
    const char* DECOR_DIR = "decor";

    void setGamePath(std::filesystem::path path) { gamePath = path; };

    std::vector<std::string> readTextFile(const std::string& path) {
        std::vector<std::string> lines;

#if defined(__ANDROID__)
        JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
        jobject activity = (jobject)SDL_AndroidGetActivity();

        if (!env || !activity) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "JNIEnv or Activity is null!");
            return {};
        }

        jclass cls = env->FindClass("com/ipoleksenko/sense/customizer/FileManager");
        if (!cls) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot find FileManager class!");
            return {};
        }

        jmethodID mid = env->GetStaticMethodID(
                cls,
                "readFullText",
                "(Landroid/content/Context;Ljava/lang/String;)Ljava/lang/String;"
        );
        if (!mid) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Method readFullText not found!");
            env->DeleteLocalRef(cls);
            return {};
        }

        jstring jfilename = env->NewStringUTF(path.c_str());
        jstring jresult = (jstring)env->CallStaticObjectMethod(cls, mid, activity, jfilename);

        if (jresult) {
            const char* chars = env->GetStringUTFChars(jresult, nullptr);
            std::string result(chars);
            env->ReleaseStringUTFChars(jresult, chars);
            env->DeleteLocalRef(jresult);

            std::istringstream iss(result);
            std::string line;
            while (std::getline(iss, line)) {
                lines.push_back(line);
            }
        } else {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "readFullText returned null for path: %s", path.c_str());
        }

        env->DeleteLocalRef(jfilename);
        env->DeleteLocalRef(cls);

#else
        try {
            std::ifstream file(path);
            if (!file.is_open()) {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Could not open file for reading: %s", path.c_str());
                return lines;
            }
            std::string line;
            while (std::getline(file, line)) {
                if (isCommentLine(line)) continue;
                lines.push_back(line);
            }
        }
        catch (const std::exception& e) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error reading file %s: %s", path.c_str(), e.what());
        }
#endif
        return lines;
    }

    bool writeTextFile(const std::string& path, const std::vector<std::string>& lines) {
#if defined(__ANDROID__)
        JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
        jobject activity = (jobject)SDL_AndroidGetActivity();

        if (!env || !activity) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get JNI environment or SDLActivity");
            return false;
        }

        jclass fileManagerClass = env->FindClass("com/ipoleksenko/sense/customizer/FileManager");
        if (!fileManagerClass) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot find FileManager class");
            return false;
        }

        jmethodID writeTextFileMethod = env->GetStaticMethodID(
                fileManagerClass,
                "writeTextFile",
                "(Landroid/content/Context;Ljava/lang/String;Ljava/util/List;)Z"
        );
        if (!writeTextFileMethod) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot find writeTextFile() method in FileManager");
            return false;
        }

        jstring jPath = env->NewStringUTF(path.c_str());

        jclass arrayListClass = env->FindClass("java/util/ArrayList");
        jmethodID arrayListCtor = env->GetMethodID(arrayListClass, "<init>", "()V");
        jmethodID arrayListAdd = env->GetMethodID(arrayListClass, "add", "(Ljava/lang/Object;)Z");
        jobject jList = env->NewObject(arrayListClass, arrayListCtor);

        for (const auto& l : lines) {
            jstring jLine = env->NewStringUTF(l.c_str());
            env->CallBooleanMethod(jList, arrayListAdd, jLine);
            env->DeleteLocalRef(jLine);
        }

        jboolean result = env->CallStaticBooleanMethod(fileManagerClass, writeTextFileMethod, activity, jPath, jList);

        env->DeleteLocalRef(jPath);
        env->DeleteLocalRef(jList);
        env->DeleteLocalRef(fileManagerClass);

        return result == JNI_TRUE;

#else
    try {
        std::ofstream file(path, std::ios::out | std::ios::trunc);
        if (!file.is_open()) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Could not open file for writing: %s", path.c_str());
            return false;
        }
        for (const auto& l : lines) {
            file << l << "\n";
        }
        return true;
    }
    catch (const std::exception& e) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error writing file %s: %s", path.c_str(), e.what());
        return false;
    }
#endif
    }

    bool fileExists(const std::string& filePath) {
        try {
#if defined(__ANDROID__)
            return callFdExistsJNI(filePath);
#else
            return std::filesystem::is_regular_file(gamePath/filePath);
#endif
        }
        catch (const std::exception& e) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error checking file existence %s: %s", filePath.c_str(), e.what());
            return false;
        }
    }

    bool dirExists(const std::string& dirPath) {
        try {
#if defined(__ANDROID__)
            return callFdExistsJNI(dirPath);
#else
            return std::filesystem::is_directory(gamePath/dirPath);
#endif
        }
        catch (const std::exception& e) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error checking directory existence %s: %s", dirPath.c_str(), e.what());
            return false;
        }
    }

    std::string extractQuotedValue(const std::string& line) {
        size_t start = line.find('"');
        if (start == std::string::npos) return "";

        std::string result;
        for (size_t i = start + 1; i < line.size(); ++i) {
            char c = line[i];
            if (c == '"') {
                size_t backslashes = 0;
                size_t j = i;
                while (j > start + 1 && line[j - 1] == '\\') {
                    ++backslashes;
                    --j;
                }
                if ((backslashes % 2) == 0) {
                    break;
                }
            }
            result.push_back(c);
        }
        return result;
    }

    std::string extractValue(const std::string& line) {
        std::string value = line;
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        return value;
    }

    bool isCommentLine(const std::string& line) {
        return !line.empty() && line[0] == '#';
    }

    std::string joinPath(const std::string& base, const std::string& path) {
        return (std::filesystem::path(base) / path).string();
    }

    std::string unescapeString(const std::string& input) {
        std::string output;
        output.reserve(input.size());

        for (size_t i = 0; i < input.size(); ++i) {
            if (input[i] == '\\' && i + 1 < input.size()) {
                char next = input[i + 1];
                switch (next) {
                case 'n':
                    output.push_back('\n');
                    ++i;
                    break;
                case 't':
                    output.append("    ");
                    ++i;
                    break;
                case '\\':
                    output.push_back('\\');
                    ++i;
                    break;
                case '"':
                    output.push_back('"');
                    ++i;
                    break;
                default:
                    output.push_back('\\');
                    output.push_back(next);
                    ++i;
                    break;
                }
            }
            else {
                output.push_back(input[i]);
            }
        }
        return output;
    }

    bool createFile(const std::string& fileName) {
#if defined(__ANDROID__)
        JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
        jobject activity = (jobject)SDL_AndroidGetActivity();

        if (!env || !activity) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "JNI environment or activity is null");
            return false;
        }

        jclass fileManagerClass = env->FindClass("com/ipoleksenko/sense/customizer/FileManager");
        if (!fileManagerClass) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "FileManager class not found");
            return false;
        }

        jmethodID createFileMethod = env->GetStaticMethodID(
                fileManagerClass,
                "createFile",
                "(Landroid/content/Context;Ljava/lang/String;)Z"
        );
        if (!createFileMethod) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "createFile method not found in FileManager");
            env->DeleteLocalRef(fileManagerClass);
            return false;
        }

        jstring jFileName = env->NewStringUTF(fileName.c_str());

        jboolean result = env->CallStaticBooleanMethod(fileManagerClass, createFileMethod, activity, jFileName);

        env->DeleteLocalRef(jFileName);
        env->DeleteLocalRef(fileManagerClass);

        return result == JNI_TRUE;
#else
    try {
        return std::filesystem::create_directories(gamePath);
    } catch (const std::exception& e) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error creating directory %s: %s", gamePath.c_str(), e.what());
        return false;
    }
#endif
    }

    // Localization
    std::map<std::string, std::string> loadLocalization() {
        std::map<std::string, std::string> result;
        std::string path = joinPath(gamePath.string(), LOCALIZATION_FILE);

        if (!fileExists(LOCALIZATION_FILE)) {
            createFile(LOCALIZATION_FILE);
        }

        for (const auto& line : readTextFile(path)) {
            size_t pos = line.find('=');
            if (pos == std::string::npos) continue;

            std::string key = line.substr(0, pos);
            std::string value = extractQuotedValue(line.substr(pos + 1));

            if (!value.empty()) {
                std::string unescaped = unescapeString(value);
                result[key] = unescaped;

                for (auto& [entryKey, entryValue] : LocalizationList) {
                    if (entryKey == key) {
                        std::snprintf(entryValue.data(), entryValue.size(), "%s", unescaped.c_str());
                        break;
                    }
                }
            }
        }

        return result;
    }

    // Custom font
    bool loadCustomFontSize() {
        std::string configPath = joinPath(gamePath.string(), FONT_FILE);
        auto lines = readTextFile(configPath);

        if (!fileExists(FONT_FILE)) {
            createFile(FONT_FILE);
        }

        if (lines.empty()) return false;

        for (const auto& line : lines) {
            // FONT="..."
            if (line.find("FONT=") == 0) {
                std::string fontPath = extractQuotedValue(line.substr(5));
                if (!fontPath.empty()) {
                    auto it = std::find_if(FontList.begin(), FontList.end(),
                        [](const auto& p) { return p.first == "FONT"; });
                    if (it != FontList.end()) {
                        std::array<char, 1024> buf{};
                        strncpy(buf.data(), fontPath.c_str(), buf.size() - 1);
                        it->second = buf;
                        SDL_Log("Using custom FONT: %s", fontPath.c_str());
                    }
                }
            }
            // FONT_SIZE=...
            else if (line.find("FONT_SIZE=") == 0) {
                std::string sizeStr = extractValue(line.substr(10));
                if (!sizeStr.empty() && std::all_of(sizeStr.begin(), sizeStr.end(), ::isdigit)) {
                    int size = std::stoi(sizeStr);
                    if (size > 0) {
                        auto it = std::find_if(FontList.begin(), FontList.end(),
                            [](const auto& p) { return p.first == "FONT_SIZE"; });
                        if (it != FontList.end()) {
                            it->second = size;
                            SDL_Log("Using custom FONT_SIZE: %d", size);
                        }
                    }
                }
            }
            // OTHER_TEXT_FONT_SIZE=...
            else if (line.find("OTHER_TEXT_FONT_SIZE=") == 0) {
                std::string sizeStr = extractValue(line.substr(21));
                if (!sizeStr.empty() && std::all_of(sizeStr.begin(), sizeStr.end(), ::isdigit)) {
                    int size = std::stoi(sizeStr);
                    if (size > 0) {
                        auto it = std::find_if(FontList.begin(), FontList.end(),
                            [](const auto& p) { return p.first == "OTHER_TEXT_FONT_SIZE"; });
                        if (it != FontList.end()) {
                            it->second = size;
                            SDL_Log("Using custom OTHER_TEXT_FONT_SIZE: %d", size);
                        }
                    }
                }
            }
        }

        return true;
    }

    std::string loadCustomFontPath() {
        std::string configPath = joinPath(gamePath.string(), FONT_FILE);

        if (!fileExists(FONT_FILE)) {
            createFile(FONT_FILE);
        }

        auto lines = readTextFile(configPath);
        for (const auto& line : lines) {
            if (line.find("FONT=") == 0) {
                std::string fontPath = extractQuotedValue(line.substr(5));
                if (!fontPath.empty()) {
                    // If path is relative, prepend working directory
                    if (fontPath[0] == '.' || !std::filesystem::path(fontPath).is_absolute()) {
                        fontPath = joinPath(gamePath.string(), fontPath);
                    }

                    if (fileExists(fontPath)) {
                        SDL_Log("Loading custom font: %s", fontPath.c_str());
                        return fontPath;
                    }
                    else {
                        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                            "Custom font file not found: %s", fontPath.c_str());
                    }
                }
            }
        }

        return "";
    }

    // Decor assets
    std::vector<DecorAsset> loadDecorAssets() {
        std::vector<DecorAsset> assets;
        std::string configPath = joinPath(gamePath.string(), DECOR_CFG);

        if (!fileExists(DECOR_CFG)) {
            createFile(DECOR_CFG);
        }

        // Read configuration
        std::map<std::string, bool> config;
        if (fileExists(configPath)) {
            auto lines = readTextFile(configPath);
            for (const auto& line : lines) {
                if (line.find('=') != std::string::npos) {
                    std::string name = line.substr(0, line.find('='));
                    std::string value = line.substr(line.find('=') + 1);
                    config[name] = (value == "true");
                }
            }
        }

        // Apply changes to the global StandartDecorList
        for (auto& [name, enabled] : StandartDecorList) {
            if (config.find(name) != config.end()) {
                enabled = config[name];
            }
            // Add updated data to assets
            assets.push_back({ name, enabled });
        }

        return assets;
    }

    void updateOrAddLine(std::vector<std::string>& lines,
        const std::string& key,
        const std::string& newValue,
        bool quoted)
    {
        bool found = false;

        std::string cleanKey = key;
        cleanKey.erase(std::remove_if(cleanKey.begin(), cleanKey.end(), ::isspace), cleanKey.end());

        std::string oneLineValue = newValue;
        {
            std::string replaced;
            replaced.reserve(oneLineValue.size());
            for (char c : oneLineValue) {
                if (c == '\n')
                    replaced += " \\n";
                else
                    replaced += c;
            }
            oneLineValue = std::move(replaced);
        }

        auto formatValue = [&](const std::string& value) -> std::string {
            return quoted ? "\"" + value + "\"" : value;
            };

        for (auto& line : lines) {
            if (line.empty() || isCommentLine(line) || line.find('=') == std::string::npos)
                continue;

            size_t eqPos = line.find('=');
            std::string leftPart = line.substr(0, eqPos);
            std::string currentKey = leftPart;
            currentKey.erase(std::remove_if(currentKey.begin(), currentKey.end(), ::isspace), currentKey.end());

            if (currentKey == cleanKey) {
                std::string rightSpaces;
                size_t i = eqPos + 1;
                while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i])))
                    rightSpaces += line[i++];

                line = leftPart + "=" + rightSpaces + formatValue(oneLineValue);
                found = true;
                break;
            }
        }

        if (!found) {
            lines.push_back(key + "=" + formatValue(oneLineValue));
        }
    }

    void updateAllConfigFiles() {
        // --- localization.cfg ---
        {
            std::string path = joinPath(gamePath.string(), LOCALIZATION_FILE);
            std::vector<std::string> lines;
            if (fileExists(path))
                lines = readTextFile(path);

            for (auto& [key, value] : LocalizationList) {
                updateOrAddLine(lines, key, std::string(value.data()), true);
            }

            writeTextFile(path, lines);
            SDL_Log("Updated localization file: %s", path.c_str());
        }

        // --- font.cfg ---
        {
            std::string path = joinPath(gamePath.string(), FONT_FILE);
            std::vector<std::string> lines;
            if (fileExists(path))
                lines = readTextFile(path);

            for (auto& [key, value] : FontList) {
                if (std::holds_alternative<int>(value))
                    updateOrAddLine(lines, key, std::to_string(std::get<int>(value)));
                else
                    updateOrAddLine(lines, key, std::string(std::get<std::array<char, 1024>>(value).data()), false);
            }

            writeTextFile(path, lines);
            SDL_Log("Updated font file: %s", path.c_str());
        }

        // --- decor.cfg ---
        {
            std::string path = joinPath(gamePath.string(), DECOR_CFG);
            std::vector<std::string> lines;
            if (fileExists(path))
                lines = readTextFile(path);

            for (auto& [key, enabled] : StandartDecorList) {
                updateOrAddLine(lines, key, enabled ? "true" : "false");
            }

            writeTextFile(path, lines);
            SDL_Log("Updated decor file: %s", path.c_str());
        }

        SDL_Log("All config files updated successfully.");
    }

    void processCustomDecorations(const std::filesystem::path& gamePath) {
        const auto decorDir = gamePath / "decor";
        SDL_Log("Scanning decor folder: %s", decorDir.string().c_str());

#if defined(__ANDROID__)
        JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
        jobject activity = (jobject)SDL_AndroidGetActivity();
        jclass fileManagerClass = env->FindClass("com/ipoleksenko/sense/customizer/FileManager");

        jmethodID copyFileMethod = env->GetStaticMethodID(
                fileManagerClass,
                "copyFile",
                "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)Z"
        );

        jmethodID deleteDecorFileMethod = env->GetStaticMethodID(
                fileManagerClass,
                "deleteDecorFile",
                "(Landroid/content/Context;Ljava/lang/String;)Z"
        );

        jmethodID renameDecorFileMethod = env->GetStaticMethodID(
                fileManagerClass,
                "renameDecorFile",
                "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)Z"
                );
#else
    if (!std::filesystem::exists(decorDir)) {
        std::filesystem::create_directories(decorDir);
        SDL_Log("Created decor directory: %s", decorDir.string().c_str());
    }
#endif

        SDL_Log("CustomDecorList size: %d", (int)CustomDecorList.size());

        for (auto& deco : CustomDecorList) {
            std::string ops;
            for (auto op : deco.operations) {
                switch (op) {
                    case CustomeDecorationOperationEnum::Add: ops += "Add "; break;
                    case CustomeDecorationOperationEnum::Remove: ops += "Remove "; break;
                    case CustomeDecorationOperationEnum::Rename: ops += "Rename "; break;
                    case CustomeDecorationOperationEnum::None: ops += "None "; break;
                }
            }
            SDL_Log("🔹 Decor: %s | path=%s | ops=[%s]",
                    deco.name.c_str(), deco.path.string().c_str(), ops.c_str());

            // ---------- ADD ----------
            if (deco.hasOperation(CustomeDecorationOperationEnum::Add)) {
#if defined(__ANDROID__)
                try {
                    jstring jSourcePath = env->NewStringUTF(deco.path.string().c_str());
                    jstring jTargetName = env->NewStringUTF((deco.name + ".png").c_str());

                    jboolean result = env->CallStaticBooleanMethod(
                            fileManagerClass,
                            copyFileMethod,
                            activity,
                            jSourcePath,
                            jTargetName
                    );

                    env->DeleteLocalRef(jSourcePath);
                    env->DeleteLocalRef(jTargetName);

                    deco.operations = { CustomeDecorationOperationEnum::None };
                } catch (...) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                                 "Exception while calling Java copyFile() for decor: %s",
                                 deco.name.c_str());
                }
#else
                try {
                auto destPath = decorDir / (deco.name + ".png");
                std::filesystem::copy_file(deco.path, destPath,
                    std::filesystem::copy_options::overwrite_existing);
                deco.path = destPath;
                deco.operations = { CustomeDecorationOperationEnum::None };
                SDL_Log("Added custom decor: %s", destPath.string().c_str());
            }
            catch (const std::exception& e) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to add decor: %s", e.what());
            }
#endif
            }

                // ---------- REMOVE ----------
            else if (deco.hasOperation(CustomeDecorationOperationEnum::Remove)) {
#if defined(__ANDROID__)
                try {
                    jstring jFilePath = env->NewStringUTF(deco.path.string().c_str());

                    jboolean deleted = env->CallStaticBooleanMethod(
                            fileManagerClass,
                            deleteDecorFileMethod,
                            activity,
                            jFilePath
                    );

                    env->DeleteLocalRef(jFilePath);

                    if (deleted == JNI_TRUE) {
                        SDL_Log("Deleted decor file via Java: %s", deco.path.string().c_str());
                    } else {
                        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                                    "Failed to delete decor via Java: %s",
                                    deco.path.string().c_str());
                    }

                    deco.operations = { CustomeDecorationOperationEnum::Remove };
                } catch (...) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                                 "Exception while calling Java deleteDecorFile() for decor: %s",
                                 deco.name.c_str());
                }
#else
                try {
                if (std::filesystem::exists(deco.path)) {
                    std::filesystem::remove(deco.path);
                    SDL_Log("🗑Removed custom decor: %s", deco.path.string().c_str());
                }
                deco.operations = { CustomeDecorationOperationEnum::Remove };
            }
            catch (const std::exception& e) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to remove decor: %s", e.what());
            }
#endif
            }

                // ---------- RENAME ----------
            else if (deco.hasOperation(CustomeDecorationOperationEnum::Rename)) {
#if defined(__ANDROID__)
                try {
                    jstring jOldFilePath = env->NewStringUTF(deco.path.string().c_str());
                    jstring jNewFileName = env->NewStringUTF((deco.name + ".png").c_str());

                    jboolean renamed = env->CallStaticBooleanMethod(
                            fileManagerClass,
                            renameDecorFileMethod,
                            activity,
                            jOldFilePath,
                            jNewFileName
                    );

                    env->DeleteLocalRef(jOldFilePath);
                    env->DeleteLocalRef(jNewFileName);

                    if (renamed == JNI_TRUE) {
                        deco.path = decorDir / (deco.name + ".png");
                        SDL_Log("Renamed decor file via Java: %s", deco.name.c_str());
                    } else {
                        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                                    "Failed to rename decor file via Java: %s",
                                    deco.name.c_str());
                    }

                    deco.operations = { CustomeDecorationOperationEnum::None };
                } catch (...) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                                 "Exception while calling Java renameDecorFile() for decor: %s",
                                 deco.name.c_str());
                }
#else
                try {
                auto newPath = decorDir / (deco.name + ".png");
                if (std::filesystem::exists(deco.path)) {
                    std::filesystem::rename(deco.path, newPath);
                    deco.path = newPath;
                    SDL_Log("Renamed decor to: %s", newPath.string().c_str());
                }
                deco.operations = { CustomeDecorationOperationEnum::None };
            }
            catch (const std::exception& e) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to rename decor: %s", e.what());
            }
#endif
            }
        }

#if defined(__ANDROID__)
        env->DeleteLocalRef(fileManagerClass);
#endif

        auto before = CustomDecorList.size();
        CustomDecorList.erase(
                std::remove_if(CustomDecorList.begin(), CustomDecorList.end(),
                               [](const CustomeDecorationList& d) {
                                   return d.hasOperation(CustomeDecorationOperationEnum::Remove);
                               }),
                CustomDecorList.end()
        );
        auto after = CustomDecorList.size();

        if (after != before) {
            SDL_Log("Removed %zu decorations from list (now %zu left).", before - after, after);
        }

        SDL_Log("Custom decorations processed.");
    }

} // namespace FileManager