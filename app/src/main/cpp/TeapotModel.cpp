#include "TeapotModel.h"
#include <cmath>

std::pair<std::vector<Vertex>, std::vector<Index>> TeapotModel::generateMesh(float scale) {
    std::vector<Vertex> vertices;
    std::vector<Index> indices;

    auto addVertex = [&](float x, float y, float z, float u, float v) {
        vertices.emplace_back(Vector3{x * scale, y * scale, z * scale}, Vector2{u, v});
        return static_cast<Index>(vertices.size() - 1);
    };

    auto addQuad = [&](Index i0, Index i1, Index i2, Index i3) {
        indices.push_back(i0);
        indices.push_back(i1);
        indices.push_back(i2);

        indices.push_back(i0);
        indices.push_back(i2);
        indices.push_back(i3);
    };

    // 1. Teapot Body (Lathe / Revolution profile)
    int stacks = 16;
    int slices = 24;

    auto getProfile = [](float t) -> std::pair<float, float> {
        float y = (t - 0.5f) * 1.6f;
        float r = 1.0f * (1.0f - 0.4f * (t - 0.4f) * (t - 0.4f));
        if (t < 0.1f) r = 0.6f + 4.0f * t * 0.4f;
        if (t > 0.85f) r = 0.7f + 0.3f * (1.0f - t);
        return {r, y};
    };

    std::vector<std::vector<Index>> grid(stacks + 1, std::vector<Index>(slices + 1));
    for (int i = 0; i <= stacks; ++i) {
        float t = (float)i / stacks;
        auto [r, y] = getProfile(t);
        for (int j = 0; j <= slices; ++j) {
            float phi = (float)j / slices * 2.0f * 3.1415926535f;
            float x = r * cosf(phi);
            float z = r * sinf(phi);
            grid[i][j] = addVertex(x, y, z, (float)j / slices, t);
        }
    }

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            addQuad(grid[i][j], grid[i + 1][j], grid[i + 1][j + 1], grid[i][j + 1]);
        }
    }

    // 2. Teapot Lid & Knob
    int lidStacks = 8;
    std::vector<std::vector<Index>> lidGrid(lidStacks + 1, std::vector<Index>(slices + 1));
    for (int i = 0; i <= lidStacks; ++i) {
        float t = (float)i / lidStacks;
        float r = 0.65f * cosf(t * 1.5707963f);
        float y = 0.8f + 0.35f * sinf(t * 1.5707963f);
        for (int j = 0; j <= slices; ++j) {
            float phi = (float)j / slices * 2.0f * 3.1415926535f;
            float x = r * cosf(phi);
            float z = r * sinf(phi);
            lidGrid[i][j] = addVertex(x, y, z, (float)j / slices, t);
        }
    }
    for (int i = 0; i < lidStacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            addQuad(lidGrid[i][j], lidGrid[i + 1][j], lidGrid[i + 1][j + 1], lidGrid[i][j + 1]);
        }
    }

    Index knobTop = addVertex(0.0f, 1.25f, 0.0f, 0.5f, 1.0f);
    Index knobRing[12];
    for (int j = 0; j < 12; ++j) {
        float phi = (float)j / 12 * 2.0f * 3.1415926535f;
        knobRing[j] = addVertex(0.12f * cosf(phi), 1.18f, 0.12f * sinf(phi), (float)j / 12, 0.8f);
    }
    for (int j = 0; j < 12; ++j) {
        int next = (j + 1) % 12;
        indices.push_back(knobTop);
        indices.push_back(knobRing[j]);
        indices.push_back(knobRing[next]);
    }

    // 3. Spout (extending +X)
    int spoutSegments = 10;
    int spoutSlices = 12;
    std::vector<std::vector<Index>> spoutGrid(spoutSegments + 1, std::vector<Index>(spoutSlices + 1));
    for (int i = 0; i <= spoutSegments; ++i) {
        float t = (float)i / spoutSegments;
        float cx = 0.7f + 0.9f * t;
        float cy = 0.0f + 0.7f * std::pow(t, 1.5f);
        float r  = 0.22f * (1.0f - 0.4f * t);

        for (int j = 0; j <= spoutSlices; ++j) {
            float angle = (float)j / spoutSlices * 2.0f * 3.1415926535f;
            float dy = r * cosf(angle);
            float dz = r * sinf(angle);
            spoutGrid[i][j] = addVertex(cx, cy + dy, dz, t, (float)j / spoutSlices);
        }
    }
    for (int i = 0; i < spoutSegments; ++i) {
        for (int j = 0; j < spoutSlices; ++j) {
            addQuad(spoutGrid[i][j], spoutGrid[i + 1][j], spoutGrid[i + 1][j + 1], spoutGrid[i][j + 1]);
        }
    }

    // 4. Handle (extending -X)
    int handleSegments = 12;
    int handleSlices = 8;
    std::vector<std::vector<Index>> handleGrid(handleSegments + 1, std::vector<Index>(handleSlices + 1));
    for (int i = 0; i <= handleSegments; ++i) {
        float t = (float)i / handleSegments;
        float theta = (t - 0.5f) * 2.6f;
        float cx = -0.8f - 0.55f * cosf(theta);
        float cy = 0.1f + 0.55f * sinf(theta);
        float r  = 0.1f;

        for (int j = 0; j <= handleSlices; ++j) {
            float angle = (float)j / handleSlices * 2.0f * 3.1415926535f;
            float dx = r * cosf(angle) * sinf(theta);
            float dy = r * cosf(angle) * cosf(theta);
            float dz = r * sinf(angle);
            handleGrid[i][j] = addVertex(cx + dx, cy + dy, dz, t, (float)j / handleSlices);
        }
    }
    for (int i = 0; i < handleSegments; ++i) {
        for (int j = 0; j < handleSlices; ++j) {
            addQuad(handleGrid[i][j], handleGrid[i + 1][j], handleGrid[i + 1][j + 1], handleGrid[i][j + 1]);
        }
    }

    return {vertices, indices};
}
