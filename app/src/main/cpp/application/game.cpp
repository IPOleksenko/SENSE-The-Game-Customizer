#include <application/game.hpp>
#include <application/window.hpp>
#include <application/renderer.hpp>
#include <objects/text.hpp>
#include <utils/icon.hpp>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

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
    SDL_Event event = {};

    bool isRunning = true;

    while (SDL_PollEvent(&event)) {}

    while (isRunning) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT: {
                isRunning = false;
                break;
            }
            case SDL_KEYDOWN: {
                const SDL_KeyboardEvent& keyboardEvent = event.key;
                if (keyboardEvent.repeat == 0) {
                    switch (keyboardEvent.keysym.sym) {
                    case SDLK_ESCAPE:
                        isRunning = false;
                        break;
                    }
                }
                break;
            }
            }
        }

        renderer.setDrawColor({ 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE });
        renderer.clear();

        renderer.present();
        SDL_Delay(16);
    }
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
