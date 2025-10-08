#include <application/game.hpp>
#include <application/window.hpp>
#include <application/renderer.hpp>
#include <objects/text.hpp>
#include <objects/imgui_window.hpp>
#include <utils/icon.hpp>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_sdlrenderer2.h>

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
    ImGui::GetIO().IniFilename = nullptr;

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    ImGui::StyleColorsDark();
    ImGui::GetIO().IniFilename = nullptr;

    ImGui_ImplSDL2_InitForSDLRenderer(window.getSdlWindow(), renderer.getSdlRenderer());
    ImGui_ImplSDLRenderer2_Init(renderer.getSdlRenderer());

    enum class Folders {
        Localization,
        Font,
        Decor,
        ImportExport
    };
    Folders currentFolder = Folders::Localization;

    ImguiWindow folderWindow;

    folderWindow.setPositionY(0);

    folderWindow.setContent([&]() {
        folderWindow.centerX();

        ImVec2 screenSize = ImGui::GetIO().DisplaySize;
        float buttonWidth = screenSize.x * 0.2f;
        float buttonHeight = screenSize.y * 0.08f;
        ImVec2 buttonSize(buttonWidth, buttonHeight);

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
        drawButton("Import/Export", Folders::ImportExport);
        
        auto folderToString = [](Folders folder) -> const char* {
            switch (folder) {
            case Folders::Localization: return "Localization";
            case Folders::Font:         return "Font";
            case Folders::Decor:        return "Decor";
            case Folders::ImportExport: return "Import/Export";
            default:                    return "Unknown";
            }
            };
        ImGui::Text("Selected folder: %s", folderToString(currentFolder));
        });

    while (isRunning) {
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            switch (event.type) {
            case SDL_QUIT: {
                isRunning = false;
                break;
            }
            case SDL_CONTROLLERDEVICEADDED: {
                int joystick_index = event.cdevice.which;
                if (SDL_IsGameController(joystick_index)) {
                    SDL_GameController* controller = SDL_GameControllerOpen(joystick_index);
                    if (controller) {
                        controllers.push_back(controller);
                        SDL_Log("Controller %d connected: %s", joystick_index, SDL_GameControllerName(controller));
                    }
                }
                break;
            }
            case SDL_CONTROLLERDEVICEREMOVED: {
                SDL_JoystickID joyId = event.cdevice.which;
                for (auto it = controllers.begin(); it != controllers.end(); ++it) {
                    if (SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(*it)) == joyId) {
                        SDL_GameControllerClose(*it);
                        controllers.erase(it);
                        SDL_Log("Controller %d disconnected", joyId);
                        break;
                    }
                }
                break;
            }
            case SDL_CONTROLLERBUTTONDOWN: {
                switch (event.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                    if (currentFolder != Folders::Localization)
                        currentFolder = static_cast<Folders>(static_cast<int>(currentFolder) - 1);
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                    if (currentFolder != Folders::ImportExport)
                        currentFolder = static_cast<Folders>(static_cast<int>(currentFolder) + 1);
                    break;
                case SDL_CONTROLLER_BUTTON_A:
                    SDL_Log("Selected folder: %d", static_cast<int>(currentFolder));
                    break;
                }
                break;
            }
            }
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
