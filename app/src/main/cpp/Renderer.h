#ifndef ANDROIDGLINVESTIGATIONS_RENDERER_H
#define ANDROIDGLINVESTIGATIONS_RENDERER_H

#include <SDL3/SDL.h>
#include <memory>
#include <vector>

#include "Model.h"
#include "Shader.h"
#include "OrientationTracker.h"
#include "TimeStep.h"
#include "RigidBody.h"
#include "PlayerShip.h"
#include "GameSettings.h"

class Renderer {
public:
    explicit Renderer(SDL_Window *window) : window_(window) {}

    virtual ~Renderer();

    /*!
     * Creates the GPU device and scene.
     * @return false if this device can't run the game's graphics; see SDL_GetError
     */
    bool init();

    void render();

private:
    bool ensureDepthTexture(Uint32 width, Uint32 height);
    void createModels();

    SDL_Window *window_;
    SDL_GPUDevice *device_ = nullptr;
    bool windowClaimed_ = false;
    SDL_GPUTexture *depthTexture_ = nullptr;
    SDL_GPUTextureFormat depthFormat_ = SDL_GPU_TEXTUREFORMAT_INVALID;
    Uint32 width_ = 0;
    Uint32 height_ = 0;

    std::unique_ptr<Shader> shader_;

    OrientationTracker orientationTracker_;
    TimeStep timeStep_;

    // Models & Player Ship
    std::unique_ptr<Model> skyboxModel_;
    std::unique_ptr<Model> teapotModel_;
    std::unique_ptr<Model> crosshairModel_;

    std::unique_ptr<PlayerShip> playerShip_;
    std::vector<std::unique_ptr<RigidBody>> floatingTeapots_;
};

#endif //ANDROIDGLINVESTIGATIONS_RENDERER_H
