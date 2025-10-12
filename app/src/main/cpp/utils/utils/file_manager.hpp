#pragma once
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <filesystem>

namespace FileManager {

void setGamePath(std::filesystem::path path);

// Basic filesystem operations
std::vector<std::string> readTextFile(const std::string& path);
bool writeTextFile(const std::string& path, const std::vector<std::string>& lines);
bool fileExists(const std::string& path);
bool dirExists(const std::string& path);
bool createDir(const std::string& path);

// Localization
struct LocalizationEntry {
    std::string key;
    std::optional<std::string> value;  // nullopt = use default text
};

std::map<std::string, std::string> loadLocalization();
bool createDefaultLocalizationFile();

// Custom font
extern int fontSize;
extern int anotherFontSize;

bool loadCustomFontSize();
std::string loadCustomFontPath();
bool createDefaultFontFile();

// Decor assets
struct DecorAsset {
    std::string name;        // File name without extension
    bool enabled = true;     // TRUE = load, FALSE = do not load
    bool isCustom = false;   // TRUE = loaded from decor folder, FALSE = standard
};

std::vector<DecorAsset> loadDecorAssets();
bool createDecorDirectory();

// Helper functions
std::string extractQuotedValue(const std::string& line);
std::string extractValue(const std::string& line);
bool isCommentLine(const std::string& line);
std::string joinPath(const std::string& base, const std::string& path);
std::string unescapeString(const std::string& input);

void updateOrAddLine(std::vector<std::string>& lines, const std::string& key, const std::string& newValue, bool quoted = false);
void updateAllConfigFiles() ;

} // namespace FileManager