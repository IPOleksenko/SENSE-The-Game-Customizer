#include <objects/imgui_window.hpp>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_sdlrenderer2.h>
#include <imgui_internal.h>
#include <assets/assets.hpp>

ImguiWindow::ImguiWindow(const std::string& title, ImGuiWindowFlags flags)
        : title(title),
          flags(flags),
          position(0, 0),
          size(0, 0),
          hasPosition(false),
          hasSize(false),
          font(nullptr),
          hasCustomFont(false)
{
    ImGui_ImplSDL2_NewFrame();
    ImGui_ImplSDLRenderer2_NewFrame();

    ImGuiIO& io = ImGui::GetIO();

    ImVec2 screenSize = io.DisplaySize;

    const float baseScreenHeight = 720.0f;
#if !defined(__ANDROID__)
    const float baseFontSize = 30.0f;
#else
    const float baseFontSize = 16.0f;
#endif
    float scale = screenSize.y / baseScreenHeight;
    float fontSize = baseFontSize * scale;

    ImFontConfig font_cfg;
    font_cfg.FontDataOwnedByAtlas = false;
    ImFont* defaultFont = io.Fonts->AddFontFromMemoryTTF(
            (void*)gFONT_FONT_TTFData,
            gFONT_FONT_TTFSize,
            fontSize,
            &font_cfg
    );

    if (defaultFont)
        font = defaultFont;
    else
        font = io.FontDefault;
}

void ImguiWindow::setPosition(const ImVec2& pos) {
    position = pos;
    hasPosition = true;
}

void ImguiWindow::setPositionX(float x) {
    position.x = x;
    hasPosition = true;
}

void ImguiWindow::setPositionY(float y) {
    position.y = y;
    hasPosition = true;
}

void ImguiWindow::setSize(const ImVec2& sz) {
    size = sz;
    hasSize = true;
}

void ImguiWindow::setSizeX(float width) {
    size.x = width;
    hasSize = true;
}

void ImguiWindow::setSizeY(float height) {
    size.y = height;
    hasSize = true;
}

void ImguiWindow::setFullscreen() {
    ImGuiIO& io = ImGui::GetIO();
    position = ImVec2(0, 0);
    size = io.DisplaySize;
    hasPosition = true;
    hasSize = true;
}

void ImguiWindow::setContent(const std::function<void()>& func) {
    contentFunc = func;
}

void ImguiWindow::setFont(ImFont* newFont) {
    font = newFont;
    hasCustomFont = true;
}

void ImguiWindow::centerX() {
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 screenSize = io.DisplaySize;
    ImVec2 currentSize = hasSize ? size : getSize();

    position.x = (screenSize.x - currentSize.x) * 0.5f;
    hasPosition = true;
}

void ImguiWindow::centerY() {
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 screenSize = io.DisplaySize;
    ImVec2 currentSize = hasSize ? size : getSize();

    position.y = (screenSize.y - currentSize.y) * 0.5f;
    hasPosition = true;
}

void ImguiWindow::center() {
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 screenSize = io.DisplaySize;
    ImVec2 scale = io.DisplayFramebufferScale;
    ImVec2 currentSize = hasSize ? size : getSize();

    position.x = ((screenSize.x * scale.x) - currentSize.x) * 0.5f;
    position.y = ((screenSize.y * scale.y) - currentSize.y) * 0.5f;
    hasPosition = true;
}

ImVec2 ImguiWindow::getPosition() const {
    ImGuiContext* ctx = ImGui::GetCurrentContext();
    if (ctx && ctx->CurrentWindow)
        return ImGui::GetWindowPos();
    return hasPosition ? position : ImVec2(0, 0);
}

ImVec2 ImguiWindow::getSize() const {
    ImGuiContext* ctx = ImGui::GetCurrentContext();
    if (ctx && ctx->CurrentWindow)
        return ImGui::GetWindowSize();
    return hasSize ? size : ImVec2(0, 0);
}

void ImguiWindow::render() {
    if (hasPosition) ImGui::SetNextWindowPos(position, ImGuiCond_Always);
    if (hasSize) ImGui::SetNextWindowSize(size, ImGuiCond_Always);

    if (ImGui::Begin(title.c_str(), nullptr, flags)) {
        if (font) ImGui::PushFont(font);
        if (contentFunc) contentFunc();
        if (font) ImGui::PopFont();
    }
    ImGui::End();
}
