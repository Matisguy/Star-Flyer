#ifndef ANDROIDGLINVESTIGATIONS_MODEL_H
#define ANDROIDGLINVESTIGATIONS_MODEL_H

#include <SDL3/SDL.h>
#include <cstdint>
#include <memory>
#include <vector>
#include "TextureAsset.h"

union Vector3 {
    struct {
        float x, y, z;
    };
    float idx[3];
};

union Vector2 {
    struct {
        float x, y;
    };
    struct {
        float u, v;
    };
    float idx[2];
};

struct Vertex {
    constexpr Vertex(const Vector3 &inPosition, const Vector2 &inUV) : position(inPosition),
                                                                       uv(inUV) {}

    Vector3 position;
    Vector2 uv;
};

typedef uint16_t Index;

class Model {
public:
    /*! Uploads the mesh to GPU buffers. */
    Model(SDL_GPUDevice *device,
          const std::vector<Vertex> &vertices,
          const std::vector<Index> &indices,
          std::shared_ptr<TextureAsset> spTexture);

    ~Model();

    Model(const Model &) = delete;
    Model &operator=(const Model &) = delete;

    inline SDL_GPUBuffer *getVertexBuffer() const {
        return vertexBuffer_;
    }

    inline SDL_GPUBuffer *getIndexBuffer() const {
        return indexBuffer_;
    }

    inline Uint32 getIndexCount() const {
        return indexCount_;
    }

    inline const TextureAsset &getTexture() const {
        return *spTexture_;
    }

private:
    SDL_GPUDevice *device_;
    SDL_GPUBuffer *vertexBuffer_;
    SDL_GPUBuffer *indexBuffer_;
    Uint32 indexCount_;
    std::shared_ptr<TextureAsset> spTexture_;
};

#endif //ANDROIDGLINVESTIGATIONS_MODEL_H
