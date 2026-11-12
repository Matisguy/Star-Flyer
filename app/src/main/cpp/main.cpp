#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <memory>

#include "GameSettings.h"
#include "Renderer.h"
#include "SensorData.h"

#ifdef SDL_PLATFORM_IOS
#include "IosPlatform.h"
#endif

// Global sensor state accessible across C++ native components
SensorData gSensorData;
std::mutex gSensorMutex;

namespace {

struct App {
    SDL_Window *window = nullptr;
    std::unique_ptr<Renderer> renderer;
    bool active = true;
};

SDL_AppResult failStartup(const char *message) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s: %s", message, SDL_GetError());
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Star Flyer", message, nullptr);
    return SDL_APP_FAILURE;
}

} // namespace

SDL_AppResult SDL_AppInit(void **appstate, int, char **) {
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return failStartup("Could not start the video system");
    }

    auto app = std::make_unique<App>();
    app->window = SDL_CreateWindow(
            "Star Flyer", 1280, 720, SDL_WINDOW_FULLSCREEN | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!app->window) {
        return failStartup("Could not create the game window");
    }

    app->renderer = std::make_unique<Renderer>(app->window);
    if (!app->renderer->init()) {
        return failStartup("This device's graphics are not supported");
    }

#ifdef SDL_PLATFORM_IOS
    // Must follow Renderer::init so the UI sits above the Metal view it creates.
    iosPlatformInstall(app->window);
#endif

    *appstate = app.release();
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    auto *app = static_cast<App *>(appstate);

    if (app && app->renderer) {
        app->renderer->handleEvent(*event);
    }

    switch (event->type) {
        case SDL_EVENT_QUIT:
        case SDL_EVENT_TERMINATING:
            return SDL_APP_SUCCESS;
        case SDL_EVENT_WILL_ENTER_BACKGROUND:
            app->active = false;
#ifdef SDL_PLATFORM_IOS
            iosPlatformSetActive(false);
#endif
            break;
        case SDL_EVENT_DID_ENTER_FOREGROUND:
            app->active = true;
#ifdef SDL_PLATFORM_IOS
            iosPlatformSetActive(true);
#endif
            break;
        default:
            break;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    auto *app = static_cast<App *>(appstate);
    // iOS terminates apps that submit GPU work while backgrounded.
    if (app->active) {
        app->renderer->render();
    }
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult) {
    auto *app = static_cast<App *>(appstate);
    if (app) {
        app->renderer.reset();
        SDL_DestroyWindow(app->window);
        delete app;
    }
}
