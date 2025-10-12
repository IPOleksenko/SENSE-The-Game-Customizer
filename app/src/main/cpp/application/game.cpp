#include <application/game.hpp>
#include <application/window.hpp>
#include <application/renderer.hpp>
#include <objects/text.hpp>
#include <objects/imgui_window.hpp>
#include <objects/missing_game_window.hpp>
#include <utils/icon.hpp>
#include <utils/find_game.hpp>
#include <utils/input_system.hpp>
#include <utils/file_manager.hpp>
#include <assets/data.hpp>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_sdlrenderer2.h>
#include <string>
#include <cstring>
#include <array>
#include <filesystem>

#if defined(__ANDROID__)

#else
#include <steam/steam_api.h>

#ifdef _WIN32
#include <windows.h>

#else
#include <cstdlib>

#endif
#endif

const std::string Game::s_orientation = "Landscape";
const std::string Game::s_name = "SENSE: The Game Customizer";
const SDL_Point Game::s_windowSize = { 1280, 720 };
const SDL_Point Game::s_windowPos = {
        static_cast<int>(SDL_WINDOWPOS_CENTERED),
        static_cast<int>(SDL_WINDOWPOS_CENTERED)
};

std::vector<SDL_GameController*> Game::controllers;

Game::Game() :
    m_isInit(false)
{
    m_isInit = (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) == 0);
    if (!m_isInit) {
        SDL_LogCritical(
            SDL_LOG_CATEGORY_SYSTEM, "%s failed: %s",
            "SDL_Init", SDL_GetError()
        );
        return;
    }

    m_isInit = (IMG_Init(IMG_INIT_PNG) != 0);
    if (!m_isInit) {
        SDL_LogCritical(
            SDL_LOG_CATEGORY_SYSTEM, "%s failed: %s",
            "IMG_Init", SDL_GetError()
        );
        return;
    }

    m_isInit = (TTF_Init() == 0);
    if (!m_isInit) {
        SDL_LogCritical(
            SDL_LOG_CATEGORY_SYSTEM, "%s failed: %s",
            "TTF_Init", SDL_GetError()
        );
        return;
    }

    if (SDL_SetHint(SDL_HINT_ORIENTATIONS, s_orientation.c_str()) == SDL_FALSE) {
        SDL_LogCritical(
            SDL_LOG_CATEGORY_SYSTEM, "%s failed: %s",
            "SDL_SetHint", "can't set orientation"
        );
    }
}

bool Game::isInit() const {
    return m_isInit;
}

void Game::run() const {
    if (!isInit()) {
        return;
    }

#if defined(__ANDROID__)
    SDL_Rect displayBounds;
    if (SDL_GetDisplayBounds(0, &displayBounds) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Failed to get display bounds: %s", SDL_GetError());
        return;
    }

    SDL_Point adjustedSize = { displayBounds.w, displayBounds.h };

    Window window(s_windowPos, adjustedSize, s_name);
    window.setFullscreenOn();
#else
    Window window(s_windowPos, s_windowSize, s_name);
#endif
    if (!window.isInit()) {
        return;
    }

    Renderer renderer(window);
    if (!renderer.isInit()) {
        return;
    }

    window.setIcon(
        Icon(SDL_Incbin(ICON_BMP))
    );

    loadStartScreen(window, renderer);
    play(window, renderer);
}

void Game::loadStartScreen(Window& window, Renderer& renderer) {
    renderer.setDrawColor({ 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE });
    renderer.clear();

    Text textLoadScreen(renderer.getSdlRenderer(), 48, { 0, 0 });
    Text textAuthor(renderer.getSdlRenderer(), 16, { 0, 0 }, true);

    textLoadScreen.setText("Loading...");
    textLoadScreen.positionCenter();
    textLoadScreen.render(window.getSize());

    textAuthor.setText("by IPOleksenko");
    textAuthor.render(window.getSize());

    renderer.present();
}

void Game::play(Window& window, Renderer& renderer) {

    SDL_Event event{};
    bool isRunning = true;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 20.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarPadding, 3.0f);

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 10.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 4.0f;
    style.ScrollbarRounding = 8.0f;
    style.WindowPadding = ImVec2(20, 20);
    style.ItemSpacing = ImVec2(12, 10);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.11f, 0.15f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.25f, 0.45f, 0.90f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.55f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.35f, 0.85f, 1.0f);
    style.Colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.0f);

    ImGui::GetIO().IniFilename = nullptr;

    ImGui_ImplSDL2_InitForSDLRenderer(window.getSdlWindow(), renderer.getSdlRenderer());
    ImGui_ImplSDLRenderer2_Init(renderer.getSdlRenderer());

    ImGui::GetIO().ConfigFlags  |= ImGuiConfigFlags_NavEnableGamepad
                                |  ImGuiBackendFlags_HasGamepad
                                |  ImGuiConfigFlags_NavEnableKeyboard
                                |  ImGuiConfigFlags_IsTouchScreen;

    FindGame findGame;
    std::filesystem::path gamePath = findGame.getGamePath();
    if (gamePath.empty() || !std::filesystem::exists(gamePath))
    {
        MissingGameWindow missingGameWindow(window, renderer);
        missingGameWindow.showMissingGameWindow(controllers);
        return;
    }

    FileManager::setGamePath(gamePath);
    FileManager::loadLocalization();
    FileManager::loadCustomFontSize();
    FileManager::loadDecorAssets();

    Folders currentFolder = Folders::Localization;

    ImguiWindow folderWindow("SENSE: The Game Customizer");
    folderWindow.setFullscreen();

    folderWindow.setFullscreen();
    folderWindow.setPosition({0,0});

    folderWindow.setContent([&]() {

        auto applyButtonStyle = [](bool isSelected) {
            if (isSelected) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.6f, 0.9f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.4f, 0.7f, 1.0f));
            }
            else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
            }
            };

        auto popButtonStyle = []() {
            ImGui::PopStyleColor(3);
            };

        ImVec2 screenSize = ImGui::GetIO().DisplaySize;

        float buttonWidth = screenSize.x * 0.19f;
#if !defined(__ANDROID__)
        float buttonHeight = screenSize.y * 0.06f;
#else
        float buttonHeight = screenSize.y * 0.1f;
#endif
        ImVec2 buttonSize(buttonWidth, buttonHeight);

        auto drawButton = [&](const char* label, Folders folderType) {
            bool isSelected = (currentFolder == folderType);

            applyButtonStyle(isSelected);

            if (ImGui::Button(label, buttonSize)) {
                currentFolder = folderType;
                SDL_Log("%s selected", label);
            }

            popButtonStyle();
            };

        drawButton("Localization", Folders::Localization);
        ImGui::SameLine();
        drawButton("Font", Folders::Font);
        ImGui::SameLine();
        drawButton("Decor", Folders::Decor);
        ImGui::SameLine();
#if !defined(__ANDROID__)
        drawButton("Import/Export", Folders::ImportExport);
        ImGui::SameLine();
        drawButton("Save/Play", Folders::SavePlay);
#else
        drawButton("Import\nExport", Folders::ImportExport);
        ImGui::SameLine();
        drawButton("Save\nPlay", Folders::SavePlay);
#endif

        static std::unordered_map<std::string, bool> cellOpen;

        if (currentFolder == Folders::Localization) {
            for (auto& [key, value] : LocalizationList) {
                if (ImGui::Button(key.c_str())) {
                    cellOpen[key] = !cellOpen[key];
                }

                if (cellOpen[key]) {
                    int newlines = 2;
                    for (char c : value) if (c == '\n') newlines++;

                    float lineHeight = ImGui::GetTextLineHeight();
                    ImVec2 inputSize(-FLT_MIN, newlines * lineHeight);

                    ImGui::InputTextMultiline(("##" + key).c_str(), value.data(), value.size(), inputSize);
                }

                ImGui::Separator();
            }
        }
        else if (currentFolder == Folders::Font) {
            for (auto& [key, value] : FontList) {
                ImGui::SeparatorText(key.c_str());

                std::visit([&](auto& val) {
                    using T = std::decay_t<decltype(val)>;

                    if constexpr (std::is_same_v<T, int>) {
                        int temp = val;

                        ImGui::SliderInt(("##slider_" + key).c_str(), &temp, 1, 100);

                        ImGui::Spacing();
                        ImGui::InputScalar(
                            ("##input_" + key).c_str(),
                            ImGuiDataType_S32,
                            &temp,
                            nullptr,
                            nullptr,
                            "%d",
                            ImGuiInputTextFlags_None
                        );

                        val = (std::max)(temp, 1);
                    }

                    else if constexpr (std::is_same_v<T, std::array<char, 1024>>) {
                        ImGui::InputTextMultiline(
                            ("##text_" + key).c_str(),
                            val.data(),
                            val.size(),
                            ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 2)
                        );
                    }
                    }, value);
            }
        }
        else if (currentFolder == Folders::Decor) {
            if (ImGui::BeginTabBar("Decor")) {

                if (ImGui::BeginTabItem("Standart Decor")) {
                    for (size_t i = 0; i < StandartDecorList.size(); ++i) {
                        auto& item = StandartDecorList[i];

                        ImGui::Checkbox(item.first.c_str(), &item.second);
                    }
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Add Custom Decor")) {
                    ImGui::EndTabItem();
                }


                ImGui::EndTabBar();
            }
        }
        else if (currentFolder == Folders::ImportExport)
        {
            ImVec2 windowSize = ImGui::GetWindowSize();

            float buttonWidth = screenSize.x * 0.4f;
            float buttonHeight = screenSize.y * 0.4f;
            float spacing = screenSize.x * 0.01f;

            float totalWidth = buttonWidth * 2 + spacing;
            float startX = (windowSize.x - totalWidth) * 0.5f;
            float startY = (windowSize.y - buttonHeight) * 0.5f;

            ImGui::SetCursorPos(ImVec2(startX, startY));

            if (ImGui::Button("Import", ImVec2(buttonWidth, buttonHeight)))
            {
                SDL_Log("Import clicked");
            }

            ImGui::SameLine(0.0f, spacing);

            if (ImGui::Button("Export", ImVec2(buttonWidth, buttonHeight)))
            {
                SDL_Log("Export clicked");
            }
        }
        else if (currentFolder == Folders::SavePlay)
        {
            ImVec2 windowSize = ImGui::GetWindowSize();

            float buttonWidth = screenSize.x * 0.5f;
            float buttonHeight = screenSize.y * 0.2f;
            float spacing = screenSize.y * 0.02f;

            float totalHeight = buttonHeight * 3 + spacing * 2;
            float startX = (windowSize.x - buttonWidth) * 0.5f;
            float startY = (windowSize.y - totalHeight) * 0.5f;

            ImGui::SetCursorPos(ImVec2(startX, startY));
            if (ImGui::Button("Save", ImVec2(buttonWidth, buttonHeight)))
                SDL_Log("Save");

            ImGui::SetCursorPos(ImVec2(startX, startY + buttonHeight + spacing));
            if (ImGui::Button("Play", ImVec2(buttonWidth, buttonHeight)))
                SDL_Log("Play");

            ImGui::SetCursorPos(ImVec2(startX, startY + (buttonHeight + spacing) * 2));
            if (ImGui::Button("Save and Play", ImVec2(buttonWidth, buttonHeight)))
                SDL_Log("Save and Play");
                }

        });

    while (isRunning) {
        ProcessSDLEvents(isRunning, controllers);

        if (!controllers.empty())
            UpdateGamepadNavigation(ImGui::GetIO(), controllers[0]);

        if (ImGui::GetIO().WantTextInput)
        {
            SDL_StartTextInput();
        }
        else
        {
            SDL_StopTextInput();
        }

        renderer.setDrawColor({ 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE });
        renderer.clear();

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        folderWindow.render();

        ImGui::Render();
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer.getSdlRenderer());

        renderer.present();
        SDL_Delay(16);
    }

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}


Game::~Game() {
    for (auto controller : controllers) {
        SDL_GameControllerClose(controller);
    }
    controllers.clear();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    m_isInit = false;
}
