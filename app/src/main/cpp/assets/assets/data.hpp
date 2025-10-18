#pragma once

#include <array>
#include <vector>
#include <string>
#include <variant>

enum class Folders {
    Localization,
    Font,
    Decor,
    ImportExport,
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

#define MAKE_ENTRY(key, text) { key, [](){ std::array<char,1024> arr{}; std::strncpy(arr.data(), text, arr.size() - 1); return arr; }() }

extern std::vector<std::pair<std::string, std::array<char, 1024>>> LocalizationList;

extern std::vector<std::pair<std::string, std::variant<int, std::array<char, 1024>>>> FontList;

extern std::vector<std::pair<std::string, bool>> StandartDecorList;
