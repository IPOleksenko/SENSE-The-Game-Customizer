#pragma once

#include <SDL.h>
#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_sdlrenderer2.h>

#include <application/window.hpp>
#include <application/renderer.hpp>
#if defined(__ANDROID__)
#include <jni.h>
#else
#include <steam/steam_api.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <cstdlib>
#endif
#endif

class MissingGameWindow
{
public:
    MissingGameWindow(Window& window, Renderer& renderer);
    ~MissingGameWindow();

    void showMissingGameWindow();

private:
    void initImGui();
    void applyStyle();
    void shutdownImGui();

private:
    Window& m_window;
    Renderer& m_renderer;
    bool m_steamRunning = false;


#if defined(__ANDROID__) 
    const char* storeName = "Google Play";
    const char* BASE_GAME_PKG_NAME = "com.ipoleksenko.sense";
    const std::string marketUri = "market://details?id=com.ipoleksenko.sense";
    std::string webFallback = "https://play.google.com/store/apps/details?id=com.ipoleksenko.sense";
#else
    const char* storeName = "Steam";
    const AppId_t BASE_GAME_APP_ID = 3832650;
    const char* STEAM_URL = "https://store.steampowered.com/app/3832650/";
    const char* STEAM_INSTALL = "steam://install/3832650";
#endif
};