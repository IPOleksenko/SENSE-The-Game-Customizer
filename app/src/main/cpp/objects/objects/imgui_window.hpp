#pragma once

#include <string>
#include <functional>
#include <imgui.h>

class ImguiWindow {
private:
    std::string title;
    ImGuiWindowFlags flags;
    ImVec2 position;
    ImVec2 size;
    bool hasPosition;
    bool hasSize;
    std::function<void()> contentFunc;
    ImFont* font;
    bool hasCustomFont;

public:
    ImguiWindow(const std::string& title = "##hidden", ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                                                                                ImGuiWindowFlags_NoResize |
                                                                                ImGuiWindowFlags_NoMove |
                                                                                ImGuiWindowFlags_NoCollapse | 
                                                                                ImGuiWindowFlags_NoBackground | ImGuiChildFlags_Border
                                                                                );
    virtual ~ImguiWindow() = default;

    void setPosition(const ImVec2& pos);
    void setPositionX(float x);
    void setPositionY(float y);
    void setSize(const ImVec2& sz);
    void setSizeX(float width);
    void setSizeY(float height);
    void setFullscreen();
    virtual void setContent(const std::function<void()>& func);
    void setFont(ImFont* newFont);
    void centerX();
    void centerY();
    void center();

    ImVec2 getPosition() const;
    ImVec2 getSize() const;

    void render();
};
