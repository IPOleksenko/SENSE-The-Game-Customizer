#pragma once

#include <array>
#include <vector>
#include <string>
#include <variant>
#include <filesystem>
#include <algorithm>
#include <SDL_image.h>

enum class Folders {
    Localization,
    Font,
    Decor,
    SavePlay
};

constexpr char IDLE_TEXT[] = (R"(Instructions:

Keyboard:
    • Press A (or the Left Arrow key) and D (or the Right Arrow key) alternately to move.
    • Press the Spacebar to enable or disable Endless Mode (only before you start moving).
    • Press the Escape key to exit the game.
)"
        #if !defined(__ANDROID__)
        R"(
    • Press F to switch between windowed and fullscreen mode.
)"
        #endif
        R"(
Gamepad:
    • Press Left or Right on the Directional Pad (or the X / B buttons) alternately to move.
    • Press the A button to enable or disable Endless Mode (only before you start moving).
    • Press the Start button to exit the game.
)"
        #if !defined(__ANDROID__)
        R"(
    • Press the Y button to switch between windowed and fullscreen mode.
)"
        #endif
        R"(
Touchscreen:
    • Tap the left and right sides of the screen alternately to move.
    • Hold two fingers on the screen for two seconds to enable or disable Endless Mode (only before you start moving).
)"
        R"(
• Cross the center mark to begin moving.
• After crossing the center, maintain your balance — do not allow the pointer to touch the red zone, or you will lose.
)"
);

#define MAKE_ENTRY(key, text) { key, [](){ std::array<char,1024> arr{}; strncpy(arr.data(), text, arr.size() - 1); return arr; }() }

extern std::vector<std::pair<std::string, std::array<char, 1024>>> LocalizationList;

extern std::vector<std::pair<std::string, std::variant<int, std::array<char, 1024>>>> FontList;

extern std::vector<std::pair<std::string, bool>> StandartDecorList;
enum class CustomeDecorationOperationEnum {
    None,
    Add,
    Remove,
    Rename
}; 
struct CustomeDecorationList {
    std::string name;
    SDL_Texture* texture = nullptr;
    std::filesystem::path path;
    std::vector<CustomeDecorationOperationEnum> operations;
    std::vector<CustomeDecorationOperationEnum> prevOperations;

    bool hasOperation(CustomeDecorationOperationEnum op) const {
        return std::find(operations.begin(), operations.end(), op) != operations.end();
    }

    void addOperation(CustomeDecorationOperationEnum op) {
        if (hasOperation(CustomeDecorationOperationEnum::Remove)) {
            if (op != CustomeDecorationOperationEnum::Remove &&
                op != CustomeDecorationOperationEnum::None)
            {
                prevOperations.erase(
                    std::remove(prevOperations.begin(), prevOperations.end(), CustomeDecorationOperationEnum::None),
                    prevOperations.end()
                );

                if (std::find(prevOperations.begin(), prevOperations.end(), op) == prevOperations.end()) {
                    prevOperations.push_back(op);
                    SDL_Log("Queued operation %d for removed decor '%s'", (int)op, name.c_str());
                }
            }

            if (op == CustomeDecorationOperationEnum::None) {
                operations.clear();
                operations.push_back(CustomeDecorationOperationEnum::None);
                prevOperations.clear();
                SDL_Log("Cleared operations for removed decor '%s'", name.c_str());
            }

            return;
        }

        if (op == CustomeDecorationOperationEnum::Remove) {
            prevOperations = operations;
            prevOperations.erase(
                std::remove(prevOperations.begin(), prevOperations.end(),
                    CustomeDecorationOperationEnum::Remove),
                prevOperations.end()
            );

            operations.clear();
            operations.push_back(CustomeDecorationOperationEnum::Remove);
            SDL_Log("Marked decor '%s' for removal", name.c_str());
            return;
        }

        if (op != CustomeDecorationOperationEnum::None) {
            operations.erase(
                std::remove(operations.begin(), operations.end(), CustomeDecorationOperationEnum::None),
                operations.end()
            );
        }

        if (op == CustomeDecorationOperationEnum::None) {
            operations.clear();
            operations.push_back(CustomeDecorationOperationEnum::None);
            SDL_Log("Reset operations for '%s'", name.c_str());
            return;
        } 
        
        std::string originalName = path.stem().string();
        if (name == originalName && hasOperation(CustomeDecorationOperationEnum::Rename)) {
            operations.erase(
                std::remove(operations.begin(), operations.end(), CustomeDecorationOperationEnum::Rename),
                operations.end()
            );
            SDL_Log("Removed rename flag: '%s' reverted to original name", name.c_str());

            if (operations.empty()) {
                operations.push_back(CustomeDecorationOperationEnum::None);
            }
            return;
        }

        if (!hasOperation(op)) {
            operations.push_back(op);
            SDL_Log("Added operation %d for '%s'", (int)op, name.c_str());
        }
    }


    void restoreFromRemove() {
        if (!prevOperations.empty()) {
            operations = prevOperations;
        }
        else {
            operations = { CustomeDecorationOperationEnum::None };
        }
        prevOperations.clear();
    }

    static bool isNameTaken(const std::string& newName,
        const std::vector<CustomeDecorationList>& list,
        const CustomeDecorationList* self = nullptr)
    {
        for (const auto& deco : list) {
            if (&deco == self) continue;
            if (deco.name == newName)
                return true;
        }
        return false;
    }

    void ensureUniqueName(std::vector<CustomeDecorationList>& list) {
        if (name.empty()) {
            name = "unknown";
        }

        sanitizeFileName(name);

        if (!isNameTaken(name, list, this))
            return;

        std::string base = name;
        int counter = 1;
        std::string newName;

        do {
            newName = base + "_new";
            if (counter > 1)
                newName += std::to_string(counter);
            counter++;
        } while (isNameTaken(newName, list, this));

        SDL_Log("Renamed duplicate '%s' → '%s'", name.c_str(), newName.c_str());
        name = newName;
        addOperation(CustomeDecorationOperationEnum::Rename);
    }

    void sanitizeFileName(std::string& s) {
        static const std::string invalidChars = "\\/:*?\"<>|";

        for (char& c : s) {
            if (invalidChars.find(c) != std::string::npos) {
                c = '_';
            }
        }
    }
};

extern std::vector<CustomeDecorationList> CustomDecorList;
