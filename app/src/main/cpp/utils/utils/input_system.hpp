#pragma once

#include <SDL.h>
#include <vector>
#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <SDL_log.h>

void UpdateGamepadNavigation(ImGuiIO& io, SDL_GameController* controller);

void ProcessSDLEvents(bool& isRunning, std::vector<SDL_GameController*>& controllers);