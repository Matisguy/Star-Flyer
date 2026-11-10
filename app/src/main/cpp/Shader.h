#ifndef ANDROIDGLINVESTIGATIONS_SHADER_H
#define ANDROIDGLINVESTIGATIONS_SHADER_H

#include <SDL3/SDL.h>
#include <memory>

class Model;

/*!
 * The game's single graphics pipeline. Inputs are a position (Vector3) and a uv (Vector2), with a
 * model/view/projection matrix, a tint color and one texture. There is no lighting.
 */
class Shader {
public:
    /*! The shader format this build ships, for requesting a compatible GPU device. */
    static SDL_GPUShaderFormat shaderFormat();

    /*! @return a valid Shader on success, otherwise null; see SDL_GetError */
    static std::unique_ptr<Shader> create(
            SDL_GPUDevice *device,
            SDL_GPUTextureFormat colorFormat,
            SDL_GPUTextureFormat depthFormat);

    ~Shader();

    /*! Prepares the shader for use, call this before executing any draw commands */
    void activate(SDL_GPUCommandBuffer *commandBuffer, SDL_GPURenderPass *renderPass);

    /*! Cleans up the shader after use, call this after executing any draw commands */
    void deactivate();

    /*! Renders a single model */
    void drawModel(const Model &model) const;

    /*!
     * Sets the model/view/projection matrix in the shader.
     * @param projectionMatrix sixteen floats, column major
     */
    void setProjectionMatrix(const float *projectionMatrix) const;

    /** Sets RGBA tint color multiplier in the shader */
    void setColor(float r, float g, float b, float a = 1.0f) const;

private:
    Shader(SDL_GPUDevice *device, SDL_GPUGraphicsPipeline *pipeline)
            : device_(device), pipeline_(pipeline) {}

    SDL_GPUDevice *device_;
    SDL_GPUGraphicsPipeline *pipeline_;
    SDL_GPUCommandBuffer *commandBuffer_ = nullptr;
    SDL_GPURenderPass *renderPass_ = nullptr;
};

#endif //ANDROIDGLINVESTIGATIONS_SHADER_H
