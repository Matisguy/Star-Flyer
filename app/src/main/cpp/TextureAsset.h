#ifndef ANDROIDGLINVESTIGATIONS_TEXTUREASSET_H
#define ANDROIDGLINVESTIGATIONS_TEXTUREASSET_H

#include <SDL3/SDL.h>
#include <memory>

class TextureAsset {
public:
    /**
     * Loads a texture asset (PNG, JPG, BMP) from the asset directory or file system
     */
    static std::shared_ptr<TextureAsset>
    loadAsset(SDL_GPUDevice *device, const std::string &assetPath);

    /**
     * Generates a procedural checkerboard texture for the skybox / placeholder rendering
     */
    static std::shared_ptr<TextureAsset>
    createCheckerboardTexture(
            SDL_GPUDevice *device,
            int width = 256,
            int height = 256,
            int checkSize = 32);

    ~TextureAsset();

    SDL_GPUTexture *getTexture() const { return texture_; }

    SDL_GPUSampler *getSampler() const { return sampler_; }

private:
    TextureAsset(SDL_GPUDevice *device, SDL_GPUTexture *texture, SDL_GPUSampler *sampler)
            : device_(device), texture_(texture), sampler_(sampler) {}

    SDL_GPUDevice *device_;
    SDL_GPUTexture *texture_;
    SDL_GPUSampler *sampler_;
};

#endif //ANDROIDGLINVESTIGATIONS_TEXTUREASSET_H
