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
        return lines;
    }

    bool writeTextFile(const std::string& path, const std::vector<std::string>& lines) {
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
    }

    bool fileExists(const std::string& path) {
        try {
            return std::filesystem::is_regular_file(path);
        }
        catch (const std::exception& e) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error checking file existence %s: %s", path.c_str(), e.what());
            return false;
        }
    }

    bool dirExists(const std::string& path) {
        try {
            return std::filesystem::is_directory(path);
        }
        catch (const std::exception& e) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error checking directory existence %s: %s", path.c_str(), e.what());
            return false;
        }
    }

    bool createDir(const std::string& path) {
        try {
            return std::filesystem::create_directories(path);
        }
        catch (const std::exception& e) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error creating directory %s: %s", path.c_str(), e.what());
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

    // Localization
    std::map<std::string, std::string> loadLocalization() {
        std::map<std::string, std::string> result;
        std::string path = joinPath(gamePath.string(), LOCALIZATION_FILE);

        if (!fileExists(path)) {
            createDefaultLocalizationFile();
            return result;
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

    bool createDefaultLocalizationFile() {
        std::string path = joinPath(gamePath.string(), LOCALIZATION_FILE);
        if (fileExists(path)) {
            return true;  // File already exists
        }

        std::vector<std::string> lines;
        lines.push_back("# SENSE: The Game Localization File");
        lines.push_back("# Use KEY=\"value\" format to override default text");
        lines.push_back("# Leave KEY= empty or \"\" to use default text");
        lines.push_back("# If KEY= is not found, default settings are applied");
        lines.push_back("# Supported escape sequences:");
        lines.push_back("#   \\n  - newline");
        lines.push_back("#   \\t  - tab (4 spaces)");
        lines.push_back("#   \\\"  - double quote");
        lines.push_back("#   \\\\  - backslash");
        lines.push_back("# Any unknown escape sequence after \\ will be written as-is");
        lines.push_back("");


        for (const auto& entry : LocalizationList) {
            const std::string& key = entry.first;
            lines.push_back(key + "=");
        }

        if (writeTextFile(path, lines)) {
            SDL_Log("Created localization config file: %s", path.c_str());
            return true;
        }
        return false;
    }

    // Custom font
    bool loadCustomFontSize() {
        std::string configPath = joinPath(gamePath.string(), FONT_FILE);
        auto lines = readTextFile(configPath);

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
            // FONT_ANOTHER_TEXT_SIZE=...
            else if (line.find("FONT_ANOTHER_TEXT_SIZE=") == 0) {
                std::string sizeStr = extractValue(line.substr(23));
                if (!sizeStr.empty() && std::all_of(sizeStr.begin(), sizeStr.end(), ::isdigit)) {
                    int size = std::stoi(sizeStr);
                    if (size > 0) {
                        auto it = std::find_if(FontList.begin(), FontList.end(),
                            [](const auto& p) { return p.first == "FONT_ANOTHER_TEXT_SIZE"; });
                        if (it != FontList.end()) {
                            it->second = size;
                            SDL_Log("Using custom FONT_ANOTHER_TEXT_SIZE: %d", size);
                        }
                    }
                }
            }
        }

        return true;
    }

    std::string loadCustomFontPath() {
        std::string configPath = joinPath(gamePath.string(), FONT_FILE);

        if (!fileExists(configPath)) {
            createDefaultFontFile();
            return "";
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

    bool createDefaultFontFile() {
        std::string path = joinPath(gamePath.string(), FONT_FILE);
        if (fileExists(path)) {
            return true;  // File already exists
        }

        std::vector<std::string> lines = {
            "# SENSE: The Game Font File",
            "# Use FONT=\"./path/to/font.ttf\" or FONT=\"font.otf\" to use a custom font",
            "# Leave FONT empty (FONT="" or FONT=) to use the default font",
            "# If the font is not found, the default font will be used",
            "# If FONT= is not found, default settings are applied",
            "FONT=\"\"",
            "",
            "# Leave FONT_SIZE= to use the default size",
            "# If FONT_SIZE is not set, the default size is 24",
            "# FONT_SIZE must contain only digits (e.g. FONT_SIZE=32)",
            "# If FONT_SIZE= is not found, default settings are applied",
            "FONT_SIZE=",
            "",
            "# Leave FONT_ANOTHER_TEXT_SIZE= to use the default size",
            "# If FONT_ANOTHER_TEXT_SIZE is not set, the default size is 48",
            "# FONT_ANOTHER_TEXT_SIZE must contain only digits (e.g. FONT_ANOTHER_TEXT_SIZE=56)",
            "# If FONT_ANOTHER_TEXT_SIZE= is not found, default settings are applied",
            "FONT_ANOTHER_TEXT_SIZE="
        };

        if (writeTextFile(path, lines)) {
            SDL_Log("Created font config file: %s", path.c_str());
            return true;
        }
        return false;
    }

    // Decor assets
    std::vector<DecorAsset> loadDecorAssets() {
        std::vector<DecorAsset> assets;
        std::string configPath = joinPath(gamePath.string(), DECOR_CFG);

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

    bool createDecorDirectory() {
        const std::string dirPath = joinPath(gamePath.string(), DECOR_DIR);
        const std::string configPath = joinPath(gamePath.string(), DECOR_CFG);

        bool success = true;

        // Create directory if it doesn't exist
        if (!dirExists(dirPath)) {
            if (createDir(dirPath)) {
                SDL_Log("Created decor directory: %s", dirPath.c_str());
            }
            else {
                success = false;
            }
        }

        // Create config file if it doesn't exist
        if (!fileExists(configPath)) {
            std::vector<std::string> lines = {
                "# SENSE: The Game Decor Configuration",
                "# Use NAME=true/false to enable/disable decor assets",
                "# If no value is set, the texture will not be displayed",
                "# Put custom .png files in the decor directory to override standard assets",
                "# You can also override a standard .png by giving it the same name",
                "# If NAME= is not found, default settings are applied",
                "",
                "# Standard decor assets:"
            };

            for (const auto& entry : StandartDecorList) {
                lines.push_back(entry.first + "=true");
            }

            if (writeTextFile(configPath, lines)) {
                SDL_Log("Created decor config file: %s", configPath.c_str());
            }
            else {
                success = false;
            }
        }

        return success;
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

} // namespace FileManager