#include <utils/input_system.hpp>


void UpdateGamepadNavigation(ImGuiIO& io, SDL_GameController* controller)
{
    if (!controller) return;

    float lx = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX) / 32767.0f;
    float ly = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY) / 32767.0f;

    auto addButton = [&](ImGuiKey key, SDL_GameControllerButton button)
        {
            bool pressed = SDL_GameControllerGetButton(controller, button);
            io.AddKeyEvent(key, pressed);
        };

    auto addAnalog = [&](ImGuiKey key, float value)
        {
            io.AddKeyAnalogEvent(key, fabs(value) > 0.3f, fabs(value));
        };

    addButton(ImGuiKey_GamepadFaceDown, SDL_CONTROLLER_BUTTON_A); // OK/Enter
    addButton(ImGuiKey_GamepadFaceRight, SDL_CONTROLLER_BUTTON_B); // Cancel
    addButton(ImGuiKey_GamepadFaceLeft, SDL_CONTROLLER_BUTTON_X); // Back
    addButton(ImGuiKey_GamepadFaceUp, SDL_CONTROLLER_BUTTON_Y);

    addButton(ImGuiKey_GamepadDpadUp, SDL_CONTROLLER_BUTTON_DPAD_UP);
    addButton(ImGuiKey_GamepadDpadDown, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    addButton(ImGuiKey_GamepadDpadLeft, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    addButton(ImGuiKey_GamepadDpadRight, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);

    addAnalog(ImGuiKey_GamepadLStickLeft, lx < -0.3f ? -lx : 0.0f);
    addAnalog(ImGuiKey_GamepadLStickRight, lx > 0.3f ? lx : 0.0f);
    addAnalog(ImGuiKey_GamepadLStickUp, ly < -0.3f ? -ly : 0.0f);
    addAnalog(ImGuiKey_GamepadLStickDown, ly > 0.3f ? ly : 0.0f);
}

void ProcessSDLEvents(bool& isRunning, std::vector<SDL_GameController*>& controllers)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);

        switch (event.type) {
        case SDL_QUIT:
            isRunning = false;
            break;

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
                    SDL_Log("Controller %d disconnected", joyId);
                    controllers.erase(it);
                    break;
                }
            }
            break;
        }

        default:
            break;
        }
    }
}
