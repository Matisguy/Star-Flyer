#include "Renderer.h"

#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <GLES3/gl3.h>
#include <memory>
#include <vector>

#include "AndroidOut.h"
#include "Shader.h"
#include "Utility.h"
#include "TextureAsset.h"
#include "TeapotModel.h"

#define PRINT_GL_STRING(s) {aout << #s": "<< glGetString(s) << std::endl;}

#define PRINT_GL_STRING_AS_LIST(s) { \
std::istringstream extensionStream((const char *) glGetString(s));\
std::vector<std::string> extensionList(\
        std::istream_iterator<std::string>{extensionStream},\
        std::istream_iterator<std::string>());\
aout << #s":\n";\
for (auto& extension: extensionList) {\
    aout << extension << "\n";\
}\
aout << std::endl;\
}

// Vertex shader
static const char *vertex = R"vertex(#version 300 es
in vec3 inPosition;
in vec2 inUV;

out vec2 fragUV;

uniform mat4 uProjection;

void main() {
    fragUV = inUV;
    gl_Position = uProjection * vec4(inPosition, 1.0);
}
)vertex";

// Fragment shader
static const char *fragment = R"fragment(#version 300 es
precision mediump float;

in vec2 fragUV;

uniform sampler2D uTexture;
uniform vec4 uColor;

out vec4 outColor;

void main() {
    outColor = texture(uTexture, fragUV) * uColor;
}
)fragment";

Renderer::~Renderer() {
    if (display_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context_ != EGL_NO_CONTEXT) {
            eglDestroyContext(display_, context_);
            context_ = EGL_NO_CONTEXT;
        }
        if (surface_ != EGL_NO_SURFACE) {
            eglDestroySurface(display_, surface_);
            surface_ = EGL_NO_SURFACE;
        }
        eglTerminate(display_);
        display_ = EGL_NO_DISPLAY;
    }
}

void Renderer::render() {
    updateRenderArea();

    // 1. Calculate timestep (delta time in seconds)
    float dt = timeStep_.tick();

    // 2. Update orientation tracker
    orientationTracker_.update(dt);

    // 3. Build 3D Perspective Projection Matrix
    float aspect = (height_ > 0) ? (float)width_ / (float)height_ : 1.0f;
    float projectionMatrix[16];
    Utility::buildPerspectiveMatrix(projectionMatrix, 65.0f, aspect, 0.1f, 500.0f);

    // 4. Build Camera View Matrix from Orientation Tracker
    float viewMatrix[16];
    orientationTracker_.getViewMatrix(viewMatrix);

    // 5. Get device look vector
    Vec3 cameraVector(viewMatrix[2], viewMatrix[6], viewMatrix[10]);

    // 6. Update Physics & Player Ship (if playing and not paused)
    bool isPlaying = (gGameSettings.gameState == GameState::PLAYING);
    bool isPaused  = (gGameSettings.gameState == GameState::PAUSED || gGameSettings.isPaused);

    if (playerShip_ && isPlaying && !isPaused) {
        playerShip_->update(dt, cameraVector, viewMatrix);

        // Update floating teapots & resolve collisions
        for (size_t i = 0; i < floatingTeapots_.size(); ++i) {
            floatingTeapots_[i]->update(dt);

            // Collisions between player ship and teapots
            RigidBody::resolveCollision(playerShip_->getRigidBody(), *floatingTeapots_[i]);

            // Collisions between teapots
            for (size_t j = i + 1; j < floatingTeapots_.size(); ++j) {
                RigidBody::resolveCollision(*floatingTeapots_[i], *floatingTeapots_[j]);
            }
        }
    } else if (!isPlaying && !isPaused) {
        // Slow ambient rotation of floating teapots when in menus
        for (size_t i = 0; i < floatingTeapots_.size(); ++i) {
            floatingTeapots_[i]->update(dt * 0.3f);
        }
    }

    // 7. Calculate Camera View Matrix: R_view = V_sensor * C
    float viewRotation[16];
    float finalViewMatrix[16];

    if (playerShip_) {
        // R_view = V_sensor * C
        float unRot[16];
        float temp[16];
        playerShip_->getRigidBody().orientation.conjugate().toMatrix(unRot);
        Utility::multiplyMatrix4x4(playerShip_->getWorldOrienter(), unRot, temp);
        Utility::multiplyMatrix4x4(viewMatrix, temp, viewRotation);

        // Include player ship position in View Matrix: V = R_view * T(-P_ship)
        Vec3 p = playerShip_->getRigidBody().position;
        for (int i = 0; i < 12; ++i) {
            finalViewMatrix[i] = viewRotation[i];
        }
        finalViewMatrix[12] = -(viewRotation[0] * p.x + viewRotation[4] * p.y + viewRotation[8]  * p.z);
        finalViewMatrix[13] = -(viewRotation[1] * p.x + viewRotation[5] * p.y + viewRotation[9]  * p.z);
        finalViewMatrix[14] = -(viewRotation[2] * p.x + viewRotation[6] * p.y + viewRotation[10] * p.z);
        finalViewMatrix[15] = 1.0f;
    } else {
        for (int i = 0; i < 16; ++i) {
            viewRotation[i] = viewMatrix[i];
            finalViewMatrix[i] = viewMatrix[i];
        }
    }

    // Clear color and depth buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // A. Render Skybox (rotation around camera only)
    if (skyboxModel_) {
        float skyboxMvp[16];
        Utility::multiplyMatrix4x4(projectionMatrix, viewRotation, skyboxMvp);
        shader_->setProjectionMatrix(skyboxMvp);
        shader_->setColor(1.0f, 1.0f, 1.0f, 1.0f);
        shader_->drawModel(*skyboxModel_);
    }

    // B. Render Floating Teapots in World Space
    if (teapotModel_ && playerShip_) {
        for (const auto &teapot : floatingTeapots_) {
            float modelMatrix[16], viewMvp[16], finalMvp[16];
            teapot->getModelMatrix(modelMatrix);
            Utility::multiplyMatrix4x4(finalViewMatrix, modelMatrix, viewMvp);
            Utility::multiplyMatrix4x4(projectionMatrix, viewMvp, finalMvp);

            shader_->setProjectionMatrix(finalMvp);
            shader_->setColor(teapot->color.x, teapot->color.y, teapot->color.z, 1.0f);
            shader_->drawModel(*teapotModel_);
        }
    }

    // C. Render Flight Direction / Ship Nose Reticle (Positioned in front of ship in world space)
    if (teapotModel_ && playerShip_) {
        float shipMatrix[16];
        playerShip_->getRigidBody().orientation.toMatrix(shipMatrix);

        // Forward look direction from ship orientation basis (-Z column)
        Vec3 forwardDir(-shipMatrix[8], -shipMatrix[9], -shipMatrix[10]);

        // Position crosshair 6.0 units directly ahead of ship in world space
        Vec3 crosshairPos = playerShip_->getRigidBody().position + forwardDir * 6.0f;

        RigidBody crosshairBody(0.0f, 0.1f, crosshairPos, Vec3(0.1f, 0.95f, 0.4f));
        crosshairBody.orientation = playerShip_->getRigidBody().orientation;
        crosshairBody.scale = 0.12f;

        float modelMatrix[16], viewMvp[16], finalMvp[16];
        crosshairBody.getModelMatrix(modelMatrix);

        Utility::multiplyMatrix4x4(finalViewMatrix, modelMatrix, viewMvp);
        Utility::multiplyMatrix4x4(projectionMatrix, viewMvp, finalMvp);

        shader_->setProjectionMatrix(finalMvp);
        shader_->setColor(0.1f, 0.95f, 0.4f, 1.0f); // Bright Neon Cyan-Green
        shader_->drawModel(*teapotModel_);
    }

    auto swapResult = eglSwapBuffers(display_, surface_);
    assert(swapResult == EGL_TRUE);
}

void Renderer::initRenderer() {
    constexpr EGLint attribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_DEPTH_SIZE, 24,
            EGL_NONE
    };

    auto display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, nullptr, nullptr);

    EGLint numConfigs;
    eglChooseConfig(display, attribs, nullptr, 0, &numConfigs);

    std::unique_ptr<EGLConfig[]> supportedConfigs(new EGLConfig[numConfigs]);
    eglChooseConfig(display, attribs, supportedConfigs.get(), numConfigs, &numConfigs);

    auto config = *std::find_if(
            supportedConfigs.get(),
            supportedConfigs.get() + numConfigs,
            [&display](const EGLConfig &config) {
                EGLint red, green, blue, depth;
                if (eglGetConfigAttrib(display, config, EGL_RED_SIZE, &red)
                    && eglGetConfigAttrib(display, config, EGL_GREEN_SIZE, &green)
                    && eglGetConfigAttrib(display, config, EGL_BLUE_SIZE, &blue)
                    && eglGetConfigAttrib(display, config, EGL_DEPTH_SIZE, &depth)) {
                    return red == 8 && green == 8 && blue == 8 && depth == 24;
                }
                return false;
            });

    EGLint format;
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
    EGLSurface surface = eglCreateWindowSurface(display, config, app_->window, nullptr);

    EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    EGLContext context = eglCreateContext(display, config, nullptr, contextAttribs);

    auto madeCurrent = eglMakeCurrent(display, surface, surface, context);
    assert(madeCurrent);

    display_ = display;
    surface_ = surface;
    context_ = context;

    width_ = -1;
    height_ = -1;

    PRINT_GL_STRING(GL_VENDOR);
    PRINT_GL_STRING(GL_RENDERER);
    PRINT_GL_STRING(GL_VERSION);
    PRINT_GL_STRING_AS_LIST(GL_EXTENSIONS);

    shader_ = std::unique_ptr<Shader>(
            Shader::loadShader(vertex, fragment, "inPosition", "inUV", "uProjection", "uColor"));
    assert(shader_);

    shader_->activate();

    // Configure OpenGL 3D state
    glClearColor(0.02f, 0.02f, 0.08f, 1.0f); // Deep space background
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // Create scene models & physics objects
    createModels();
}

void Renderer::updateRenderArea() {
    EGLint width;
    eglQuerySurface(display_, surface_, EGL_WIDTH, &width);

    EGLint height;
    eglQuerySurface(display_, surface_, EGL_HEIGHT, &height);

    if (width != width_ || height != height_) {
        width_ = width;
        height_ = height;
        glViewport(0, 0, width, height);

        shaderNeedsNewProjectionMatrix_ = true;
    }
}

void Renderer::createModels() {
    // 1. Player Ship Physics Object
    playerShip_ = std::make_unique<PlayerShip>();

    // 2. Skybox Cube Model
    float s = 100.0f;
    float t = 8.0f;
    std::vector<Vertex> skyboxVertices = {
        // Front Face (+Z)
        Vertex(Vector3{-s, -s,  s}, Vector2{0, 0}),
        Vertex(Vector3{ s, -s,  s}, Vector2{t, 0}),
        Vertex(Vector3{ s,  s,  s}, Vector2{t, t}),
        Vertex(Vector3{-s,  s,  s}, Vector2{0, t}),

        // Back Face (-Z)
        Vertex(Vector3{ s, -s, -s}, Vector2{0, 0}),
        Vertex(Vector3{-s, -s, -s}, Vector2{t, 0}),
        Vertex(Vector3{-s,  s, -s}, Vector2{t, t}),
        Vertex(Vector3{ s,  s, -s}, Vector2{0, t}),

        // Left Face (-X)
        Vertex(Vector3{-s, -s, -s}, Vector2{0, 0}),
        Vertex(Vector3{-s, -s,  s}, Vector2{t, 0}),
        Vertex(Vector3{-s,  s,  s}, Vector2{t, t}),
        Vertex(Vector3{-s,  s, -s}, Vector2{0, t}),

        // Right Face (+X)
        Vertex(Vector3{ s, -s,  s}, Vector2{0, 0}),
        Vertex(Vector3{ s, -s, -s}, Vector2{t, 0}),
        Vertex(Vector3{ s,  s, -s}, Vector2{t, t}),
        Vertex(Vector3{ s,  s,  s}, Vector2{0, t}),

        // Top Face (+Y)
        Vertex(Vector3{-s,  s,  s}, Vector2{0, 0}),
        Vertex(Vector3{ s,  s,  s}, Vector2{t, 0}),
        Vertex(Vector3{ s,  s, -s}, Vector2{t, t}),
        Vertex(Vector3{-s,  s, -s}, Vector2{0, t}),

        // Bottom Face (-Y)
        Vertex(Vector3{-s, -s, -s}, Vector2{0, 0}),
        Vertex(Vector3{ s, -s, -s}, Vector2{t, 0}),
        Vertex(Vector3{ s, -s,  s}, Vector2{t, t}),
        Vertex(Vector3{-s, -s,  s}, Vector2{0, t}),
    };

    std::vector<Index> skyboxIndices;
    for (int i = 0; i < 6; ++i) {
        Index offset = static_cast<Index>(i * 4);
        skyboxIndices.push_back(offset + 0);
        skyboxIndices.push_back(offset + 1);
        skyboxIndices.push_back(offset + 2);

        skyboxIndices.push_back(offset + 0);
        skyboxIndices.push_back(offset + 2);
        skyboxIndices.push_back(offset + 3);
    }

    auto checkerboardTexture = TextureAsset::createCheckerboardTexture(256, 256, 32);
    skyboxModel_ = std::make_unique<Model>(skyboxVertices, skyboxIndices, checkerboardTexture);

    // 3. Utah Teapot Mesh
    auto [teapotVertices, teapotIndices] = TeapotModel::generateMesh(1.0f);
    teapotModel_ = std::make_unique<Model>(teapotVertices, teapotIndices, checkerboardTexture);

    // 4. Spawn 25 floating teapots scattered in space
    floatingTeapots_.clear();
    for (int i = 0; i < 25; ++i) {
        float angle = (float)i / 25.0f * 6.2831853f;
        float dist = 20.0f + (i % 5) * 15.0f;
        float x = cosf(angle) * dist + ((i % 3) - 1) * 8.0f;
        float y = sinf(angle * 2.0f) * 10.0f + ((i % 4) - 2) * 5.0f;
        float z = -15.0f - (i * 6.0f);

        float mass = 1.0f + (i % 6) * 5.0f; // 1kg to 26kg
        float radius = 1.2f + (i % 4) * 0.4f;

        Vec3 colors[] = {
            Vec3(0.95f, 0.78f, 0.3f), // Gold
            Vec3(0.85f, 0.45f, 0.2f), // Copper
            Vec3(0.35f, 0.4f, 0.5f),  // Slate
            Vec3(0.9f, 0.9f, 0.95f),  // Silver
            Vec3(0.85f, 0.25f, 0.25f),// Crimson
            Vec3(0.2f, 0.85f, 0.95f)  // Cyan
        };
        Vec3 color = colors[i % 6];

        auto teapot = std::make_unique<RigidBody>(mass, radius, Vec3(x, y, z), color);
        teapot->scale = radius;
        teapot->angularVelocity = Vec3((i % 3 - 1) * 0.2f, (i % 2 - 0.5f) * 0.4f, (i % 4 - 2) * 0.15f);
        floatingTeapots_.push_back(std::move(teapot));
    }
}

void Renderer::handleInput() {
    auto *inputBuffer = android_app_swap_input_buffers(app_);
    if (!inputBuffer) {
        return;
    }

    android_app_clear_motion_events(inputBuffer);
    android_app_clear_key_events(inputBuffer);
}
