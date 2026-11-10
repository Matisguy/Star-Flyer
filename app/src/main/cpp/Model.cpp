#include "Model.h"

#include <cstring>

namespace {

SDL_GPUBuffer *uploadBuffer(
        SDL_GPUDevice *device,
        SDL_GPUBufferUsageFlags usage,
        const void *data,
        Uint32 size) {
    SDL_GPUBufferCreateInfo bufferInfo = {};
    bufferInfo.usage = usage;
    bufferInfo.size = size;
    SDL_GPUBuffer *buffer = SDL_CreateGPUBuffer(device, &bufferInfo);

    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = size;
    SDL_GPUTransferBuffer *transfer = SDL_CreateGPUTransferBuffer(device, &transferInfo);

    void *mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
    SDL_memcpy(mapped, data, size);
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_GPUCommandBuffer *commandBuffer = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(commandBuffer);

    SDL_GPUTransferBufferLocation source = {transfer, 0};
    SDL_GPUBufferRegion destination = {buffer, 0, size};
    SDL_UploadToGPUBuffer(copyPass, &source, &destination, false);

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);
    SDL_ReleaseGPUTransferBuffer(device, transfer);

    return buffer;
}

} // namespace

Model::Model(
        SDL_GPUDevice *device,
        const std::vector<Vertex> &vertices,
        const std::vector<Index> &indices,
        std::shared_ptr<TextureAsset> spTexture)
        : device_(device),
          vertexBuffer_(uploadBuffer(
                  device,
                  SDL_GPU_BUFFERUSAGE_VERTEX,
                  vertices.data(),
                  static_cast<Uint32>(vertices.size() * sizeof(Vertex)))),
          indexBuffer_(uploadBuffer(
                  device,
                  SDL_GPU_BUFFERUSAGE_INDEX,
                  indices.data(),
                  static_cast<Uint32>(indices.size() * sizeof(Index)))),
          indexCount_(static_cast<Uint32>(indices.size())),
          spTexture_(std::move(spTexture)) {}

Model::~Model() {
    SDL_ReleaseGPUBuffer(device_, vertexBuffer_);
    SDL_ReleaseGPUBuffer(device_, indexBuffer_);
}
