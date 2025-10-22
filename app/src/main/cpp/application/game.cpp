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
#include <jni.h>
#else
#include <steam/steam_api.h>
#include <tinyfiledialogs.h>

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

#if defined(__ANDROID__)
static std::vector<CustomeDecorationList> gPendingDecorations;
static std::mutex gPendingMutex;

static std::unordered_map<std::string, std::vector<unsigned char>> gImageCache;

void ProcessPendingDecorations(SDL_Renderer* renderer)
{
    std::vector<CustomeDecorationList> localQueue;

    {
        std::lock_guard<std::mutex> lock(gPendingMutex);
        if (gPendingDecorations.empty()) return;
        localQueue.swap(gPendingDecorations);
    }

    for (auto& deco : localQueue)
    {
        auto it = gImageCache.find(deco.path.string());
        if (it == gImageCache.end()) {
            SDL_Log("No cached image bytes for: %s", deco.path.c_str());
            continue;
        }

        const auto& bytes = it->second;

        SDL_RWops* rw = SDL_RWFromConstMem(bytes.data(), bytes.size());
        if (!rw) {
            SDL_Log("SDL_RWFromConstMem failed for %s: %s", deco.path.c_str(), SDL_GetError());
            continue;
        }

        SDL_Surface* surface = IMG_Load_RW(rw, 1);
        if (!surface) {
            SDL_Log("IMG_Load_RW failed for %s: %s", deco.path.c_str(), IMG_GetError());
            continue;
        }

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);

        if (!texture) {
            SDL_Log("SDL_CreateTextureFromSurface failed for %s", deco.path.c_str());
            continue;
        }

        deco.texture = texture;
        deco.ensureUniqueName(CustomDecorList);
        CustomDecorList.push_back(std::move(deco));

        SDL_Log("Decor texture added: %s", deco.path.c_str());

        gImageCache.erase(it);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ipoleksenko_sense_customizer_FileManager_nativeOnImagePicked(
        JNIEnv* env, jclass, jstring jPath, jbyteArray byteArray)
{
    if (!byteArray || !jPath) return;

    const char* rawPath = env->GetStringUTFChars(jPath, nullptr);
    std::string path(rawPath);
    env->ReleaseStringUTFChars(jPath, rawPath);

    jsize len = env->GetArrayLength(byteArray);
    std::vector<unsigned char> bytes(len);
    env->GetByteArrayRegion(byteArray, 0, len, reinterpret_cast<jbyte*>(bytes.data()));

    CustomeDecorationList item;
    item.path = path;
    item.name = std::filesystem::path(path).stem().string();
    item.operations.push_back(CustomeDecorationOperationEnum::Add);

    {
        std::lock_guard<std::mutex> lock(gPendingMutex);
        gPendingDecorations.push_back(std::move(item));
        gImageCache[path] = std::move(bytes);
    }

    SDL_Log("Received image %s (%d bytes) from Java", path.c_str(), (int)len);
}
#endif

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

const void Game::launchGame() {
#if defined(__ANDROID__)
    const char* packageName = "com.ipoleksenko.sense";
    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
    jobject activity = (jobject)SDL_AndroidGetActivity();

    jclass activityClass = env->GetObjectClass(activity);
    if (!activityClass) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get activity class");
        return;
    }

    jmethodID getPackageManager = env->GetMethodID(activityClass,
                                                   "getPackageManager", "()Landroid/content/pm/PackageManager;");
    jobject packageManager = env->CallObjectMethod(activity, getPackageManager);

    jclass pmClass = env->GetObjectClass(packageManager);
    jmethodID getLaunchIntentForPackage = env->GetMethodID(pmClass,
                                                           "getLaunchIntentForPackage", "(Ljava/lang/String;)Landroid/content/Intent;");
    jstring jPackageName = env->NewStringUTF(packageName);
    jobject launchIntent = env->CallObjectMethod(packageManager, getLaunchIntentForPackage, jPackageName);

    if (launchIntent) {
        jmethodID startActivity = env->GetMethodID(activityClass, "startActivity", "(Landroid/content/Intent;)V");
        env->CallVoidMethod(activity, startActivity, launchIntent);
        SDL_Log("Launching installed game: %s", packageName);
    } else {
        jclass uriClass = env->FindClass("android/net/Uri");
        jmethodID parse = env->GetStaticMethodID(uriClass, "parse", "(Ljava/lang/String;)Landroid/net/Uri;");
        std::string marketUrl = "market://details?id=" + std::string(packageName);
        jstring jUrl = env->NewStringUTF(marketUrl.c_str());
        jobject uri = env->CallStaticObjectMethod(uriClass, parse, jUrl);

        jclass intentClass = env->FindClass("android/content/Intent");
        jmethodID intentConstructor = env->GetMethodID(intentClass, "<init>",
                                                       "(Ljava/lang/String;Landroid/net/Uri;)V");
        jstring actionView = env->NewStringUTF("android.intent.action.VIEW");
        jobject marketIntent = env->NewObject(intentClass, intentConstructor, actionView, uri);

        jmethodID startActivity = env->GetMethodID(activityClass, "startActivity", "(Landroid/content/Intent;)V");
        env->CallVoidMethod(activity, startActivity, marketIntent);
        SDL_Log("Game not installed, opening Google Play: %s", marketUrl.c_str());

        env->DeleteLocalRef(uri);
        env->DeleteLocalRef(jUrl);
        env->DeleteLocalRef(uriClass);
        env->DeleteLocalRef(intentClass);
        env->DeleteLocalRef(actionView);
        env->DeleteLocalRef(marketIntent);
    }

    env->DeleteLocalRef(jPackageName);
    env->DeleteLocalRef(pmClass);
    env->DeleteLocalRef(packageManager);
    env->DeleteLocalRef(activityClass);
    env->DeleteLocalRef(activity);
#else
    const char* steamAppId = "3832650";
    std::string steamCommand = "steam://run/" + std::string(steamAppId);

#ifdef _WIN32
    ShellExecuteA(nullptr, "open", steamCommand.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#elif defined(__APPLE__)
    std::string command = "open \"" + steamCommand + "\"";
    system(command.c_str());
#else
    std::string command = "xdg-open \"" + steamCommand + "\"";
    system(command.c_str());
#endif

    SDL_Log("Launching Steam game: %s", steamCommand.c_str());
#endif
}

void Game::AddCustomDecorFromDialog(SDL_Renderer* renderer)
{
#if defined(__ANDROID__)
    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
    if (!env) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get JNI environment");
        return;
    }

    jclass fileManagerClass = env->FindClass("com/ipoleksenko/sense/customizer/FileManager");
    if (!fileManagerClass) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to find class: com/ipoleksenko/sense/customizer/FileManager");
        return;
    }

    jmethodID pickImageMethod = env->GetStaticMethodID(fileManagerClass, "pickImageFromGallery", "()V");
    if (!pickImageMethod) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to find static method: pickImageFromGallery()V");
        env->DeleteLocalRef(fileManagerClass);
        return;
    }

    env->CallStaticVoidMethod(fileManagerClass, pickImageMethod);
    SDL_Log("Called FileManager.pickImageFromGallery() successfully");

    env->DeleteLocalRef(fileManagerClass);
#else
    const char* filters[] = { "*.png" };
    const char* selectedPath = tinyfd_openFileDialog(
        "Select PNG image",
        "",
        1,
        filters,
        "PNG images",
        0 // 0 = single file, 1 = allow multiple selection
    );

    if (!selectedPath)
    {
        SDL_Log("No file selected");
        return;
    }

    SDL_Log("Selected file: %s", selectedPath);

    SDL_Texture* tex = IMG_LoadTexture(renderer, selectedPath);
    if (!tex)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load texture: %s", IMG_GetError());
        return;
    }

    CustomeDecorationList item;
    item.name = std::filesystem::path(selectedPath).stem().string();
    item.path = selectedPath;
    item.texture = tex;
    item.operations.push_back(CustomeDecorationOperationEnum::Add);

    item.ensureUniqueName(CustomDecorList);

    CustomDecorList.push_back(std::move(item));

    SDL_Log("Added custom decor: %s", selectedPath);
#endif
}

void Game::DrawAddCustomDecorTab(SDL_Renderer* renderer, const std::filesystem::path& gamePath)
{
    if (gamePath.empty() || !std::filesystem::exists(gamePath)) {
        ImGui::TextColored(ImVec4(1, 0.2f, 0.2f, 1), "Game path not found");
        return;
    }

    static std::filesystem::path s_loadedPath;
    if (s_loadedPath != gamePath) {
        for (auto& it : CustomDecorList) {
            if (it.texture) SDL_DestroyTexture(it.texture);
        }
        CustomDecorList.clear();

        std::filesystem::path decorPath = gamePath / "decor";
        if (!std::filesystem::exists(decorPath))
            decorPath = gamePath;

        SDL_Log("Scanning decor folder: %s", decorPath.string().c_str());

        for (auto& entry : std::filesystem::directory_iterator(decorPath)) {
            if (!entry.is_regular_file()) continue;
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".png") {
                SDL_Texture* tex = IMG_LoadTexture(renderer, entry.path().string().c_str());
                if (!tex) continue;

                CustomeDecorationList item;
                item.name = entry.path().stem().string();
                item.path = entry.path();
                item.texture = tex;
                item.operations.push_back(CustomeDecorationOperationEnum::None);
                CustomDecorList.push_back(std::move(item));
            }
        }

        s_loadedPath = gamePath;
    }

    const char* buttonText = "Add PNG for add assets";
    ImVec2 textSize = ImGui::CalcTextSize(buttonText);
    ImVec2 buttonSize(textSize.x + 80.0f, 80.0f);

    float windowWidth = ImGui::GetWindowSize().x;
    float cursorX = (windowWidth - buttonSize.x) * 0.5f;
    if (cursorX < 0.0f) cursorX = 0.0f;
    ImGui::SetCursorPosX(cursorX);

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 25.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.40f, 0.90f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.50f, 1.00f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.35f, 0.80f, 1.00f));

    bool addClicked = ImGui::Button(buttonText, buttonSize);

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();

    if (addClicked) {
        AddCustomDecorFromDialog(renderer);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Folder: %s", gamePath.string().c_str());
    ImGui::Text("Loaded items: %d", (int)CustomDecorList.size());
    ImGui::Spacing();

    if (CustomDecorList.empty()) {
        ImGui::TextDisabled("No PNG assets found.");
        return;
    }

    int idx = 0;
    for (size_t i = 0; i < CustomDecorList.size(); ++i) {
        auto& item = CustomDecorList[i];
        ImGui::PushID(idx++);

        bool isRemoved = item.hasOperation(CustomeDecorationOperationEnum::Remove);
        bool isNew = item.hasOperation(CustomeDecorationOperationEnum::Add);

        if (isRemoved) {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
        }

        ImGui::Separator();
        ImGui::Text("Name:");
        ImGui::SameLine();

        char buf[1024];
        std::snprintf(buf, sizeof(buf), "%s", item.name.c_str());
        if (ImGui::InputText(("##name" + std::to_string(idx)).c_str(), buf, sizeof(buf))) {
            if (item.name != buf) {
                item.name = buf;
                item.ensureUniqueName(CustomDecorList);
                item.addOperation(CustomeDecorationOperationEnum::Rename);
            }

        }

        ImGui::TextWrapped("Path: %s", item.path.string().c_str());

        if (item.texture) {
            int texW = 0, texH = 0;
            SDL_QueryTexture(item.texture, nullptr, nullptr, &texW, &texH);
            float maxWidth = 256.0f;
            float scale = (texW > 0) ? ((texW > maxWidth) ? (maxWidth / (float)texW) : 1.0f) : 1.0f;
            ImVec2 imageSize(texW * scale, texH * scale);
            ImGui::Image((ImTextureID)(intptr_t)item.texture, imageSize);
        }

        ImGui::SameLine();

        if (isRemoved) {
            if (ImGui::Button(("Restore##" + std::to_string(idx)).c_str())) {
                item.restoreFromRemove();
                SDL_Log("Restored item: %s", item.name.c_str());

            }
        }
        else {
            const char* btnLabel = isNew ? "Cancel##" : "Remove##";
            if (ImGui::Button((std::string(btnLabel) + std::to_string(idx)).c_str())) {
                if (isNew) {
                    if (item.texture) SDL_DestroyTexture(item.texture);
                    CustomDecorList.erase(CustomDecorList.begin() + i);
                    SDL_Log("Canceled new item: %s", item.name.c_str());
                    ImGui::PopID();
                    break;
                }
                else {
                    item.addOperation(CustomeDecorationOperationEnum::Remove);
                    SDL_Log("Marked for removal: %s", item.name.c_str());
                }
            }
        }

        if (!item.operations.empty()) {
            ImGui::Text("Operations:");
            for (auto op : item.operations) {
                const char* label = "";
                switch (op) {
                case CustomeDecorationOperationEnum::None:   label = "None"; break;
                case CustomeDecorationOperationEnum::Add:    label = "Add"; break;
                case CustomeDecorationOperationEnum::Remove: label = "Remove"; break;
                case CustomeDecorationOperationEnum::Rename: label = "Rename"; break;
                }
                ImGui::SameLine();
                ImGui::Text("[%s]", label);
            }
        }

        if (isRemoved) {
            ImGui::PopStyleVar();
        }

        ImGui::PopID();
    }
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
#if defined(__ANDROID__)
    FileManager::setGamePath("");
#else
    FileManager::setGamePath(gamePath);
#endif
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

                        ImGui::SliderInt(("##slider_" + key).c_str(), &temp, 1, 128);

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

                        val = std::clamp(temp, 1, 128);
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
                    DrawAddCustomDecorTab(renderer.getSdlRenderer(), gamePath);
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
            if (ImGui::Button("Save", ImVec2(buttonWidth, buttonHeight))) {
                FileManager::updateAllConfigFiles();
                FileManager::processCustomDecorations(gamePath);
            }

            ImGui::SetCursorPos(ImVec2(startX, startY + buttonHeight + spacing));
            if (ImGui::Button("Play", ImVec2(buttonWidth, buttonHeight))) {
                launchGame();
            }

            ImGui::SetCursorPos(ImVec2(startX, startY + (buttonHeight + spacing) * 2));
            if (ImGui::Button("Save and Play", ImVec2(buttonWidth, buttonHeight))) {
                FileManager::updateAllConfigFiles();
                FileManager::processCustomDecorations(gamePath);
                launchGame();
            }
        }

        });

    while (isRunning) {
        ProcessSDLEvents(isRunning, controllers);
#if defined(__ANDROID__)
        ProcessPendingDecorations(renderer.getSdlRenderer());
#endif
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
