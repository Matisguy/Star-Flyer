#ifndef ANDROIDGLINVESTIGATIONS_RENDERER_H
#define ANDROIDGLINVESTIGATIONS_RENDERER_H

#include <EGL/egl.h>
#include <memory>
#include <vector>

#include "Model.h"
#include "Shader.h"
#include "OrientationTracker.h"
#include "TimeStep.h"
#include "RigidBody.h"
#include "PlayerShip.h"
#include "GameSettings.h"

struct android_app;

class Renderer {
public:
    /*!
     * @param pApp the android_app this Renderer belongs to, needed to configure GL
     */
    inline Renderer(android_app *pApp) :
            app_(pApp),
            display_(EGL_NO_DISPLAY),
            surface_(EGL_NO_SURFACE),
            context_(EGL_NO_CONTEXT),
            width_(0),
            height_(0),
            shaderNeedsNewProjectionMatrix_(true) {
        initRenderer();
    }

    virtual ~Renderer();

    void handleInput();
    void render();

private:
    void initRenderer();
    void updateRenderArea();
    void createModels();

    android_app *app_;
    EGLDisplay display_;
    EGLSurface surface_;
    EGLContext context_;
    EGLint width_;
    EGLint height_;

    bool shaderNeedsNewProjectionMatrix_;

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
