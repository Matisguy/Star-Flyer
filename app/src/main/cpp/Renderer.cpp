#include "Renderer.h"

#include <cmath>
#include <memory>
#include <vector>

#include "Shader.h"
#include "Utility.h"
#include "TextureAsset.h"
#include "TeapotModel.h"

Renderer::~Renderer() {
    if (!device_) {
        return;
    }

    SDL_WaitForGPUIdle(device_);

    skyboxModel_.reset();
    teapotModel_.reset();
    crosshairModel_.reset();
    shader_.reset();

    if (depthTexture_) {
        SDL_ReleaseGPUTexture(device_, depthTexture_);
    }
    if (windowClaimed_) {
        SDL_ReleaseWindowFromGPUDevice(device_, window_);
    }
    SDL_DestroyGPUDevice(device_);
}

void Renderer::handleEvent(const SDL_Event &event) {
    tapInputHandler_.handleEvent(event, width_, height_);
}

void Renderer::render() {
    SDL_GPUCommandBuffer *commandBuffer = SDL_AcquireGPUCommandBuffer(device_);
    if (!commandBuffer) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Command buffer failed: %s", SDL_GetError());
        return;
    }

    SDL_GPUTexture *swapchainTexture = nullptr;
    Uint32 swapchainWidth = 0;
    Uint32 swapchainHeight = 0;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(
            commandBuffer, window_, &swapchainTexture, &swapchainWidth, &swapchainHeight)) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Swapchain failed: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(commandBuffer);
        return;
    }

    // No drawable surface right now, e.g. while Android recreates it after backgrounding.
    if (!swapchainTexture || !ensureDepthTexture(swapchainWidth, swapchainHeight)) {
        SDL_SubmitGPUCommandBuffer(commandBuffer);
        return;
    }


    // 1. Calculate timestep (delta time in seconds)
    float dt = timeStep_.tick();

    // 2. Dynamically update Skybox Texture based on GameState / Active Level
    // - On Startup & Menus: level1_2skybox.png
    // - On Mission 0 (Debug Level): tutorialskybox.png
    std::string targetSkybox = ((gGameSettings.gameState == GameState::PLAYING) || (gGameSettings.gameState == GameState::PAUSED))
            ? "tutorialskybox.png"
            : "level1_2skybox.png";
    updateSkybox(targetSkybox);

    // 3. Update orientation tracker
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
        for (size_t i = 0; i < gameObjects_.size(); ++i) {
            gameObjects_[i]->update(dt);

            // Collisions between player ship and teapots
            RigidBody::resolveCollision(playerShip_->getRigidBody(), *gameObjects_[i]);

            // Collisions between teapots
            for (size_t j = i + 1; j < gameObjects_.size(); ++j) {
                RigidBody::resolveCollision(*gameObjects_[i], *gameObjects_[j]);
            }
        }
    } else if (!isPlaying && !isPaused) {
        // Slow ambient rotation of floating teapots when in menus
        for (size_t i = 0; i < gameObjects_.size(); ++i) {
            gameObjects_[i]->update(dt * 0.3f);
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

    // 8. Update Tap Input Handler (raycasting, target pointing, shotAt destruction, hitmarkers)
    tapInputHandler_.update(dt, static_cast<int>(width_), static_cast<int>(height_), finalViewMatrix, projectionMatrix, gameObjects_);

    // Clear color and depth buffers
    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture = swapchainTexture;
    colorTarget.clear_color = {0.02f, 0.02f, 0.08f, 1.0f}; // Deep space background
    colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPUDepthStencilTargetInfo depthTarget = {};
    depthTarget.texture = depthTexture_;
    depthTarget.clear_depth = 1.0f;
    depthTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    depthTarget.store_op = SDL_GPU_STOREOP_DONT_CARE;
    depthTarget.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
    depthTarget.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
    depthTarget.cycle = true;

    SDL_GPURenderPass *renderPass =
            SDL_BeginGPURenderPass(commandBuffer, &colorTarget, 1, &depthTarget);
    shader_->activate(commandBuffer, renderPass);

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
        for (const auto &teapot : gameObjects_) {
            float modelMatrix[16], viewMvp[16], finalMvp[16];
            teapot->getModelMatrix(modelMatrix);
            Utility::multiplyMatrix4x4(finalViewMatrix, modelMatrix, viewMvp);
            Utility::multiplyMatrix4x4(projectionMatrix, viewMvp, finalMvp);

            shader_->setProjectionMatrix(finalMvp);
            if (teapot->pointedAt) {
                shader_->setColor(0.1f, 1.0f, 0.4f, 1.0f); // Bright Neon Green Target Highlight
            } else {
                shader_->setColor(teapot->color.x, teapot->color.y, teapot->color.z, 1.0f);
            }
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

    // D. Render Tap Intersecting Crosshair Lines (Vertical & Horizontal)
    if (tapInputHandler_.isTapActive()) {
        float tapX = 0.0f, tapY = 0.0f;
        tapInputHandler_.getTapPosition(tapX, tapY);

        // Vertical Line (top to bottom across screen)
        drawHudLine(tapX, 0.0f, tapX, static_cast<float>(height_), 3.0f, 0.1f, 1.0f, 0.4f, 1.0f);

        // Horizontal Line (left to right across screen)
        drawHudLine(0.0f, tapY, static_cast<float>(width_), tapY, 3.0f, 0.1f, 1.0f, 0.4f, 1.0f);
    }

    // E. Render Hitmarker Effect (0.05 seconds duration on tap release)
    const auto &hitmarker = tapInputHandler_.getHitmarker();
    if (hitmarker.timeRemaining > 0.0f) {
        if (!hitmarkerTexture_) {
            hitmarkerTexture_ = TextureAsset::loadAsset(device_, "hitmarkerdefault.png");
        }
        drawHudQuad(hitmarker.x, hitmarker.y, 48.0f, 48.0f, hitmarkerTexture_, 1.0f, 0.2f, 0.2f, 1.0f);
    }

    shader_->deactivate();
    SDL_EndGPURenderPass(renderPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);
}

void Renderer::drawHudLine(float x1, float y1, float x2, float y2, float thickness, float r, float g, float b, float a) {
    if (!device_ || width_ == 0 || height_ == 0) return;

    float ortho2D[16];
    Utility::buildOrthographicMatrix(ortho2D, static_cast<float>(height_) * 0.5f, static_cast<float>(width_) / static_cast<float>(height_), 0.0f, 10.0f);

    float dx = x2 - x1;
    float dy = y2 - y1;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.0001f) return;

    float nx = -dy / len * (thickness * 0.5f);
    float ny = dx / len * (thickness * 0.5f);

    float halfW = static_cast<float>(width_) * 0.5f;
    float halfH = static_cast<float>(height_) * 0.5f;

    auto mapX = [halfW](float x) { return x - halfW; };
    auto mapY = [halfH](float y) { return halfH - y; };

    std::vector<Vertex> lineVertices = {
        Vertex(Vector3{mapX(x1 + nx), mapY(y1 + ny), 0.0f}, Vector2{0.0f, 0.0f}),
        Vertex(Vector3{mapX(x1 - nx), mapY(y1 - ny), 0.0f}, Vector2{1.0f, 0.0f}),
        Vertex(Vector3{mapX(x2 - nx), mapY(y2 - ny), 0.0f}, Vector2{1.0f, 1.0f}),
        Vertex(Vector3{mapX(x2 + nx), mapY(y2 + ny), 0.0f}, Vector2{0.0f, 1.0f}),
    };
    std::vector<Index> lineIndices = {0, 1, 2, 0, 2, 3};

    auto checkerboard = TextureAsset::createCheckerboardTexture(device_, 64, 64, 32);
    Model tempLineModel(device_, lineVertices, lineIndices, checkerboard);

    shader_->setProjectionMatrix(ortho2D);
    shader_->setColor(r, g, b, a);
    shader_->drawModel(tempLineModel);
}

void Renderer::drawHudQuad(float centerX, float centerY, float w, float h, std::shared_ptr<TextureAsset> texture, float r, float g, float b, float a) {
    if (!device_ || width_ == 0 || height_ == 0) return;

    float ortho2D[16];
    Utility::buildOrthographicMatrix(ortho2D, static_cast<float>(height_) * 0.5f, static_cast<float>(width_) / static_cast<float>(height_), 0.0f, 10.0f);

    float halfW = static_cast<float>(width_) * 0.5f;
    float halfH = static_cast<float>(height_) * 0.5f;

    auto mapX = [halfW](float x) { return x - halfW; };
    auto mapY = [halfH](float y) { return halfH - y; };

    float x0 = mapX(centerX - w * 0.5f);
    float x1 = mapX(centerX + w * 0.5f);
    float y0 = mapY(centerY - h * 0.5f);
    float y1 = mapY(centerY + h * 0.5f);

    std::vector<Vertex> quadVertices = {
        Vertex(Vector3{x0, y0, 0.0f}, Vector2{0.0f, 0.0f}),
        Vertex(Vector3{x1, y0, 0.0f}, Vector2{1.0f, 0.0f}),
        Vertex(Vector3{x1, y1, 0.0f}, Vector2{1.0f, 1.0f}),
        Vertex(Vector3{x0, y1, 0.0f}, Vector2{0.0f, 1.0f}),
    };
    std::vector<Index> quadIndices = {0, 1, 2, 0, 2, 3};

    if (!texture) {
        texture = TextureAsset::createCheckerboardTexture(device_, 64, 64, 32);
    }

    Model tempQuadModel(device_, quadVertices, quadIndices, texture);

    shader_->setProjectionMatrix(ortho2D);
    shader_->setColor(r, g, b, a);
    shader_->drawModel(tempQuadModel);
}

bool Renderer::init() {
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN,
                           Shader::shaderFormat() == SDL_GPU_SHADERFORMAT_SPIRV);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_MSL_BOOLEAN,
                           Shader::shaderFormat() == SDL_GPU_SHADERFORMAT_MSL);
#ifndef NDEBUG
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, true);
#endif
    // The game uses none of these, and requiring them would exclude some Android GPUs.
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_CLIP_DISTANCE_BOOLEAN, false);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_DEPTH_CLAMPING_BOOLEAN, false);
    SDL_SetBooleanProperty(
            props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_INDIRECT_DRAW_FIRST_INSTANCE_BOOLEAN, false);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_ANISOTROPY_BOOLEAN, false);

    device_ = SDL_CreateGPUDeviceWithProperties(props);
    SDL_DestroyProperties(props);
    if (!device_) {
        return false;
    }

    if (!SDL_ClaimWindowForGPUDevice(device_, window_)) {
        return false;
    }
    windowClaimed_ = true;

    SDL_Log("GPU driver: %s", SDL_GetGPUDeviceDriver(device_));

    depthFormat_ = SDL_GPUTextureSupportsFormat(
            device_,
            SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
            SDL_GPU_TEXTURETYPE_2D,
            SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET)
                   ? SDL_GPU_TEXTUREFORMAT_D32_FLOAT
                   : SDL_GPU_TEXTUREFORMAT_D24_UNORM;

    shader_ = Shader::create(
            device_, SDL_GetGPUSwapchainTextureFormat(device_, window_), depthFormat_);
    if (!shader_) {
        return false;
    }

    // Create scene models & physics objects
    createModels();
    return true;
}

bool Renderer::ensureDepthTexture(Uint32 width, Uint32 height) {
    if (depthTexture_ && width == width_ && height == height_) {
        return true;
    }

    if (depthTexture_) {
        SDL_ReleaseGPUTexture(device_, depthTexture_);
    }

    SDL_GPUTextureCreateInfo info = {};
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = depthFormat_;
    info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    info.width = width;
    info.height = height;
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    depthTexture_ = SDL_CreateGPUTexture(device_, &info);

    width_ = width;
    height_ = height;
    return depthTexture_ != nullptr;
}

namespace {

std::pair<std::vector<Vertex>, std::vector<Index>> createSkyboxNetMesh(float s) {
    float u0 = 0.0f, u1 = 0.25f, u2 = 0.5f, u3 = 0.75f, u4 = 1.0f;
    float v0 = 0.0f, v1 = 1.0f / 3.0f, v2 = 2.0f / 3.0f, v3 = 1.0f;

    std::vector<Vertex> vertices = {
        // 1. Front Face (-Z) - Column 1, Middle Row (u1..u2, v1..v2)
        Vertex(Vector3{s,  s,  -s}, Vector2{u1, v2}),
        Vertex(Vector3{-s, s,  -s}, Vector2{u2, v2}),
        Vertex(Vector3{-s,-s,  -s}, Vector2{u2, v1}),
        Vertex(Vector3{s, -s,  -s}, Vector2{u1, v1}),

        // 2. Back Face (+Z) - Column 3, Middle Row (u3..u4, v1..v2)
        Vertex(Vector3{-s, s, s}, Vector2{u3, v2}),
        Vertex(Vector3{s,  s, s}, Vector2{u4, v2}),
        Vertex(Vector3{s, -s, s}, Vector2{u4, v1}),
        Vertex(Vector3{-s,-s, s}, Vector2{u3, v1}),

        // 3. Left Face (+X) - Column 0, Middle Row (u0..u1, v1..v2)
        Vertex(Vector3{s, s, s}, Vector2{u0, v2}),
        Vertex(Vector3{s, s,-s}, Vector2{u1, v2}),
        Vertex(Vector3{s,-s,-s}, Vector2{u1, v1}),
        Vertex(Vector3{s,-s, s}, Vector2{0, v1}),

        // 4. Right Face (-X) - Column 2, Middle Row (u2..u3, v1..v2)
        Vertex(Vector3{ -s, s,-s}, Vector2{u2, v2}),
        Vertex(Vector3{ -s, s, s}, Vector2{u3, v2}),
        Vertex(Vector3{ -s,-s, s}, Vector2{u3, v1}),
        Vertex(Vector3{ -s,-s,-s}, Vector2{u2, v1}),

        // 5. Top / Upper Face (-Y) - Column 1, Top Row (u1..u2, v0..v1)
        Vertex(Vector3{s,  -s,-s}, Vector2{u1, v1}),
        Vertex(Vector3{-s, -s,-s}, Vector2{u2, v1}),
        Vertex(Vector3{-s, -s, s}, Vector2{u2, v0}),
        Vertex(Vector3{s,  -s, s}, Vector2{u1, v0}),

        // 6. Bottom / Lower Face (+Y) - Column 1, Bottom Row (u1..u2, v2..v3)
        Vertex(Vector3{s, s, s}, Vector2{u1, v3}),
        Vertex(Vector3{-s,s, s}, Vector2{u2, v3}),
        Vertex(Vector3{-s,s,-s}, Vector2{u2, v2}),
        Vertex(Vector3{s, s,-s}, Vector2{u1, v2}),
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

    return {vertices, indices};
}

} // namespace

void Renderer::updateSkybox(const std::string &textureName) {
    if (currentSkyboxName_ == textureName && skyboxModel_) {
        return;
    }

    currentSkyboxName_ = textureName;
    auto [skyboxVertices, skyboxIndices] = createSkyboxNetMesh(100.0f);

    // Load custom asset texture (e.g. level1_2skybox.png or tutorialskybox.png)
    auto texture = TextureAsset::loadAsset(device_, textureName);
    if (!texture) {
        // Fallback to checkerboard texture if file is not found in asset directory
        texture = TextureAsset::createCheckerboardTexture(device_, 256, 256, 32);
    }

    skyboxModel_ = std::make_unique<Model>(device_, skyboxVertices, skyboxIndices, texture);
}

void Renderer::createModels() {
    // 1. Player Ship Physics Object
    playerShip_ = std::make_unique<PlayerShip>();

    // 2. Initialize Skybox Model with default startup texture (level1_2skybox.png)
    updateSkybox("level1_2skybox.png");

    // 3. Utah Teapot Mesh
    auto checkerboardTexture = TextureAsset::createCheckerboardTexture(device_, 256, 256, 32);
    auto [teapotVertices, teapotIndices] = TeapotModel::generateMesh(1.0f);
    teapotModel_ = std::make_unique<Model>(device_, teapotVertices, teapotIndices, checkerboardTexture);

    // 4. Spawn 25 floating teapots scattered in space
    gameObjects_.clear();
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
        teapot->type = 0;
        teapot->scale = radius;
        teapot->angularVelocity = Vec3((i % 3 - 1) * 0.2f, (i % 2 - 0.5f) * 0.4f, (i % 4 - 2) * 0.15f);
        gameObjects_.push_back(std::move(teapot));
    }
}
