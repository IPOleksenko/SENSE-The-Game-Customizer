#include <objects/missing_game_window.hpp>


MissingGameWindow::MissingGameWindow(Window& window, Renderer& renderer)
    : m_window(window), m_renderer(renderer) {
}

MissingGameWindow::~MissingGameWindow() = default;

void MissingGameWindow::applyStyle()
{
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
}

void MissingGameWindow::initImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    applyStyle();

    ImGui_ImplSDL2_InitForSDLRenderer(m_window.getSdlWindow(), m_renderer.getSdlRenderer());
    ImGui_ImplSDLRenderer2_Init(m_renderer.getSdlRenderer());
}

void MissingGameWindow::shutdownImGui()
{
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void MissingGameWindow::showMissingGameWindow()
{
    SDL_Event event{};
    bool isRunning = true;

    initImGui();

#if !defined(__ANDROID__)
    m_steamRunning = SteamAPI_Init();
#endif

    while (isRunning)
    {
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                isRunning = false;
        }

        m_renderer.setDrawColor({ 15, 15, 20, 255 });
        m_renderer.clear();

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        bool ownsGame = false;
        bool isInstalled = false;

#if !defined(__ANDROID__)
        if (m_steamRunning && SteamUser() && SteamApps())
        {
            ownsGame = SteamApps()->BIsSubscribedApp(BASE_GAME_APP_ID);
            isInstalled = SteamApps()->BIsAppInstalled(BASE_GAME_APP_ID);
        }
        else
        {
            if (!m_steamRunning)
                m_steamRunning = SteamAPI_Init();
        }
#else
        ownsGame = true;

        JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
        jobject activity = (jobject)SDL_AndroidGetActivity();
        if (env && activity) {
            jclass activityClass = env->GetObjectClass(activity);
            jmethodID getPackageManager = env->GetMethodID(activityClass, "getPackageManager", "()Landroid/content/pm/PackageManager;");
            jobject pm = env->CallObjectMethod(activity, getPackageManager);

            jclass pmClass = env->GetObjectClass(pm);
            jmethodID getPackageInfo = env->GetMethodID(pmClass, "getPackageInfo",
                                                        "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;");

            jstring packageName = env->NewStringUTF(BASE_GAME_PKG_NAME);

            jobject pkgInfo = nullptr;
            jthrowable exc = nullptr;

            pkgInfo = env->CallObjectMethod(pm, getPackageInfo, packageName, 0);
            exc = env->ExceptionOccurred();

            if (exc) {
                env->ExceptionClear();
                isInstalled = false;
            } else {
                isInstalled = true;
            }

            if (pkgInfo) env->DeleteLocalRef(pkgInfo);
            env->DeleteLocalRef(packageName);
            env->DeleteLocalRef(pmClass);
            env->DeleteLocalRef(pm);
            env->DeleteLocalRef(activityClass);
        } else {
            isInstalled = false;
        }
#endif

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("Missing Game", nullptr,
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoSavedSettings);

        ImVec2 screen = ImGui::GetIO().DisplaySize;
        ImVec2 center(screen.x * 0.5f, screen.y * 0.5f);
        ImGui::SetCursorPos(ImVec2(center.x - 200, center.y - 120));

#if !defined(__ANDROID__)
        if (!m_steamRunning)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.4f, 1.0f),
                "%s is not running.\nPlease start %s and restart this program.", storeName, storeName);
        }
        else
#endif
            if (!ownsGame)
            {
                ImGui::TextWrapped("You don't own the base game.\nPlease purchase it on %s to continue.", storeName);

                ImVec2 btn(260, 70);
                ImGui::SetCursorPos(ImVec2(center.x - btn.x / 2, center.y + 20));

                std::string openStoreButton = std::string("Open ") + storeName + " Store Page";
                if (ImGui::Button(openStoreButton.c_str(), btn))
                {
#if defined(__ANDROID__)
                    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
                    jobject activity = (jobject)SDL_AndroidGetActivity();
                    if (env && activity)
                    {
                        jclass uriClass = env->FindClass("android/net/Uri");
                        jmethodID parse = env->GetStaticMethodID(uriClass, "parse", "(Ljava/lang/String;)Landroid/net/Uri;");
                        jstring uriString = env->NewStringUTF(marketUri.c_str());
                        jobject uriObj = env->CallStaticObjectMethod(uriClass, parse, uriString);

                        jclass intentClass = env->FindClass("android/content/Intent");
                        jmethodID ctor = env->GetMethodID(intentClass, "<init>", "(Ljava/lang/String;Landroid/net/Uri;)V");
                        jstring actionView = env->NewStringUTF("android.intent.action.VIEW");
                        jobject intent = env->NewObject(intentClass, ctor, actionView, uriObj);

                        jclass activityClass = env->GetObjectClass(activity);
                        jmethodID startActivity = env->GetMethodID(activityClass, "startActivity", "(Landroid/content/Intent;)V");
                        env->CallVoidMethod(activity, startActivity, intent);

                        env->DeleteLocalRef(intent);
                        env->DeleteLocalRef(actionView);
                        env->DeleteLocalRef(intentClass);
                        env->DeleteLocalRef(uriObj);
                        env->DeleteLocalRef(uriString);
                        env->DeleteLocalRef(uriClass);
                        env->DeleteLocalRef(activityClass);
                    }
#else
#ifdef _WIN32
                    ShellExecuteA(nullptr, "open", STEAM_URL, nullptr, nullptr, SW_SHOWNORMAL);
#else
                    system((std::string("xdg-open ") + STEAM_URL).c_str());
#endif
#endif
                }
            }
            else if (!isInstalled)
            {
                ImGui::TextWrapped("The game is not installed.\nPlease install it from %s to continue.", storeName);

                ImVec2 btn(240, 70);
                ImGui::SetCursorPos(ImVec2(center.x - btn.x / 2, center.y + 20));

                std::string installButton = std::string("Install via ") + storeName;
                if (ImGui::Button(installButton.c_str(), btn))
                {
#if defined(__ANDROID__)
                    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
                    jobject activity = (jobject)SDL_AndroidGetActivity();
                    if (env && activity)
                    {
                        jclass uriClass = env->FindClass("android/net/Uri");
                        jmethodID parse = env->GetStaticMethodID(uriClass, "parse", "(Ljava/lang/String;)Landroid/net/Uri;");
                        jstring uriString = env->NewStringUTF(marketUri.c_str());
                        jobject uriObj = env->CallStaticObjectMethod(uriClass, parse, uriString);

                        jclass intentClass = env->FindClass("android/content/Intent");
                        jmethodID ctor = env->GetMethodID(intentClass, "<init>", "(Ljava/lang/String;Landroid/net/Uri;)V");
                        jstring actionView = env->NewStringUTF("android.intent.action.VIEW");
                        jobject intent = env->NewObject(intentClass, ctor, actionView, uriObj);

                        jclass activityClass = env->GetObjectClass(activity);
                        jmethodID startActivity = env->GetMethodID(activityClass, "startActivity", "(Landroid/content/Intent;)V");
                        env->CallVoidMethod(activity, startActivity, intent);

                        env->DeleteLocalRef(intent);
                        env->DeleteLocalRef(actionView);
                        env->DeleteLocalRef(intentClass);
                        env->DeleteLocalRef(uriObj);
                        env->DeleteLocalRef(uriString);
                        env->DeleteLocalRef(uriClass);
                        env->DeleteLocalRef(activityClass);
                    }
#else
#ifdef _WIN32
                    ShellExecuteA(nullptr, "open", STEAM_INSTALL, nullptr, nullptr, SW_SHOWNORMAL);
#else
                    system((std::string("xdg-open ") + STEAM_INSTALL).c_str());
#endif
#endif
                }
            }
            else
            {
                ImGui::TextWrapped("The game is installed and ready to use!\nPlease restart the application to continue.");

                ImVec2 btn(200, 60);
                ImGui::SetCursorPos(ImVec2(center.x - btn.x / 2, center.y + 20));
                if (ImGui::Button("Close Application", btn))
                    isRunning = false;
            }

        ImGui::End();
        ImGui::Render();
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), m_renderer.getSdlRenderer());
        m_renderer.present();

#if !defined(__ANDROID__)
        if (m_steamRunning)
            SteamAPI_RunCallbacks();
#endif

        SDL_Delay(16);
    }

#if !defined(__ANDROID__)
    if (m_steamRunning)
        SteamAPI_Shutdown();
#endif

    shutdownImGui();
}
