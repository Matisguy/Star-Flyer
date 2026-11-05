#include "Renderer.h"

#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <GLES3/gl3.h>
#include <memory>
#include <vector>

#include "AndroidOut.h"
#include "Shader.h"
#include "Utility.h"
#include "TextureAsset.h"
#include "SensorData.h"

//! executes glGetString and outputs the result to logcat
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

out vec4 outColor;

void main() {
    outColor = texture(uTexture, fragUV);
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

    // 2. Update orientation tracker with sensor fusion & noise reduction
    orientationTracker_.update(dt);

    // 3. Build 3D perspective projection matrix (65° Field of View)
    float aspect = (height_ > 0) ? (float)width_ / (float)height_ : 1.0f;
    float projectionMatrix[16];
    Utility::buildPerspectiveMatrix(projectionMatrix, 65.0f, aspect, 0.1f, 500.0f);

    // 4. Retrieve smooth 3D camera view matrix from OrientationTracker
    float viewMatrix[16];
    orientationTracker_.getViewMatrix(viewMatrix);

    // 5. Combine Projection * View into Model-View-Projection (MVP) matrix
    float mvpMatrix[16];
    Utility::multiplyMatrix4x4(projectionMatrix, viewMatrix, mvpMatrix);

    // Upload MVP matrix to shader
    shader_->setProjectionMatrix(mvpMatrix);

    // Clear color and depth buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw skybox models
    if (!models_.empty()) {
        for (const auto &model: models_) {
            shader_->drawModel(model);
        }
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
            Shader::loadShader(vertex, fragment, "inPosition", "inUV", "uProjection"));
    assert(shader_);

    shader_->activate();

    // Configure OpenGL 3D state
    glClearColor(0.02f, 0.02f, 0.08f, 1.0f); // Deep space background
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // Create checkerboard skybox world
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
    float s = 50.0f; // Skybox cube radius/size
    float t = 4.0f;  // Texture repeat factor (4x4 checkerboard grid per face)

    std::vector<Vertex> vertices = {
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

    std::vector<Index> indices;
    for (int i = 0; i < 6; ++i) {
        Index offset = static_cast<Index>(i * 4);
        indices.push_back(offset + 0);
        indices.push_back(offset + 1);
        indices.push_back(offset + 2);

        indices.push_back(offset + 0);
        indices.push_back(offset + 2);
        indices.push_back(offset + 3);
    }

    // Generate procedural checkerboard texture
    auto checkerboardTexture = TextureAsset::createCheckerboardTexture(256, 256, 32);
    models_.emplace_back(vertices, indices, checkerboardTexture);
}

void Renderer::handleInput() {
    auto *inputBuffer = android_app_swap_input_buffers(app_);
    if (!inputBuffer) {
        return;
    }

    android_app_clear_motion_events(inputBuffer);
    android_app_clear_key_events(inputBuffer);
}