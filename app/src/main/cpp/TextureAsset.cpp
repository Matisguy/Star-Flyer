#include "TextureAsset.h"

#include <cstdint>
#include <vector>

std::shared_ptr<TextureAsset>
TextureAsset::createCheckerboardTexture(
        SDL_GPUDevice *device, int width, int height, int checkSize) {
    std::vector<uint8_t> pixels(width * height * 4);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            bool isEven = ((x / checkSize) + (y / checkSize)) % 2 == 0;
            // High contrast sci-fi grid: deep navy/grey vs bright cyan/white
            uint8_t r = isEven ? 25 : 190;
            uint8_t g = isEven ? 35 : 210;
            uint8_t b = isEven ? 60 : 235;
            uint8_t a = 255;

            int idx = (y * width + x) * 4;
            pixels[idx + 0] = r;
            pixels[idx + 1] = g;
            pixels[idx + 2] = b;
            pixels[idx + 3] = a;
        }
    }

    Uint32 mipLevels = 1;
    for (int size = SDL_max(width, height); size > 1; size /= 2) {
        ++mipLevels;
    }

    SDL_GPUTextureCreateInfo textureInfo = {};
    textureInfo.type = SDL_GPU_TEXTURETYPE_2D;
    textureInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    // Mipmap generation renders into the texture, so it must also be a color target.
    textureInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
    textureInfo.width = width;
    textureInfo.height = height;
    textureInfo.layer_count_or_depth = 1;
    textureInfo.num_levels = mipLevels;
    SDL_GPUTexture *texture = SDL_CreateGPUTexture(device, &textureInfo);

    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = static_cast<Uint32>(pixels.size());
    SDL_GPUTransferBuffer *transfer = SDL_CreateGPUTransferBuffer(device, &transferInfo);

    void *mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
    SDL_memcpy(mapped, pixels.data(), pixels.size());
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_GPUCommandBuffer *commandBuffer = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(commandBuffer);

    SDL_GPUTextureTransferInfo source = {};
    source.transfer_buffer = transfer;
    source.pixels_per_row = width;
    source.rows_per_layer = height;

    SDL_GPUTextureRegion destination = {};
    destination.texture = texture;
    destination.w = width;
    destination.h = height;
    destination.d = 1;

    SDL_UploadToGPUTexture(copyPass, &source, &destination, false);
    SDL_EndGPUCopyPass(copyPass);
    SDL_GenerateMipmapsForGPUTexture(commandBuffer, texture);
    SDL_SubmitGPUCommandBuffer(commandBuffer);
    SDL_ReleaseGPUTransferBuffer(device, transfer);

    SDL_GPUSamplerCreateInfo samplerInfo = {};
    samplerInfo.min_filter = SDL_GPU_FILTER_LINEAR;
    samplerInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
    samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    // Repeat mode for tiled skybox faces
    samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    samplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    // Zero would clamp sampling to the top mip level.
    samplerInfo.max_lod = static_cast<float>(mipLevels);
    SDL_GPUSampler *sampler = SDL_CreateGPUSampler(device, &samplerInfo);

    return std::shared_ptr<TextureAsset>(new TextureAsset(device, texture, sampler));
}

TextureAsset::~TextureAsset() {
    SDL_ReleaseGPUSampler(device_, sampler_);
    SDL_ReleaseGPUTexture(device_, texture_);
}
