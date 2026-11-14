#include "PolyhedraModel.h"
#include <cmath>
#include <vector>

namespace {

void addNormalizedVertex(std::vector<Vertex> &vertices, float x, float y, float z, float scale) {
    float len = std::sqrt(x * x + y * y + z * z);
    if (len > 0.00001f) {
        x = (x / len) * scale;
        y = (y / len) * scale;
        z = (z / len) * scale;
    }
    // Calculate spherical UV coordinates for texture mapping
    float u = 0.5f + std::atan2(z, x) / (2.0f * 3.1415926535f);
    float v = 0.5f - std::asin(std::max(-1.0f, std::min(1.0f, y / scale))) / 3.1415926535f;

    vertices.emplace_back(Vector3{x, y, z}, Vector2{u, v});
}

} // namespace

// 1. ICOSAHEDRON (20 faces, 12 vertices)
std::pair<std::vector<Vertex>, std::vector<Index>> PolyhedraModel::generateIcosahedron(float scale) {
    std::vector<Vertex> vertices;
    std::vector<Index> indices;

    float phi = (1.0f + std::sqrt(5.0f)) * 0.5f;

    float rawVerts[12][3] = {
        {-1,  phi, 0}, { 1,  phi, 0}, {-1, -phi, 0}, { 1, -phi, 0},
        { 0, -1,  phi}, { 0,  1,  phi}, { 0, -1, -phi}, { 0,  1, -phi},
        { phi, 0, -1}, { phi, 0,  1}, {-phi, 0, -1}, {-phi, 0,  1}
    };

    for (int i = 0; i < 12; ++i) {
        addNormalizedVertex(vertices, rawVerts[i][0], rawVerts[i][1], rawVerts[i][2], scale);
    }

    Index rawIndices[20][3] = {
        {0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11},
        {1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
        {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9},
        {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}
    };

    for (int i = 0; i < 20; ++i) {
        indices.push_back(rawIndices[i][0]);
        indices.push_back(rawIndices[i][1]);
        indices.push_back(rawIndices[i][2]);
    }

    return {vertices, indices};
}

// 2. RHOMBIC DODECAHEDRON (12 rhombic faces, 14 vertices)
std::pair<std::vector<Vertex>, std::vector<Index>> PolyhedraModel::generateRhombicDodecahedron(float scale) {
    std::vector<Vertex> vertices;
    std::vector<Index> indices;

    float rawVerts[14][3] = {
        // 8 Cube Vertices
        {-1, -1, -1}, { 1, -1, -1}, { 1,  1, -1}, {-1,  1, -1},
        {-1, -1,  1}, { 1, -1,  1}, { 1,  1,  1}, {-1,  1,  1},
        // 6 Octahedron Vertices
        {-2,  0,  0}, { 2,  0,  0}, { 0, -2,  0}, { 0,  2,  0}, { 0,  0, -2}, { 0,  0,  2}
    };

    for (int i = 0; i < 14; ++i) {
        addNormalizedVertex(vertices, rawVerts[i][0], rawVerts[i][1], rawVerts[i][2], scale);
    }

    // 12 Rhombic faces (each defined by 4 vertices)
    Index rawQuads[12][4] = {
        {13, 4, 8, 7}, {13, 5, 9, 6}, {13, 7, 11, 6}, {13, 4, 10, 5},
        {12, 0, 8, 3}, {12, 1, 9, 2}, {12, 3, 11, 2}, {12, 0, 10, 1},
        {8, 4, 10, 0}, {8, 7, 11, 3}, {9, 5, 10, 1}, {9, 6, 11, 2}
    };

    for (int i = 0; i < 12; ++i) {
        indices.push_back(rawQuads[i][0]);
        indices.push_back(rawQuads[i][1]);
        indices.push_back(rawQuads[i][2]);

        indices.push_back(rawQuads[i][0]);
        indices.push_back(rawQuads[i][2]);
        indices.push_back(rawQuads[i][3]);
    }

    return {vertices, indices};
}

// 3. RHOMBICUBOCTAHEDRON (26 faces: 8 triangles + 18 squares, 24 vertices)
std::pair<std::vector<Vertex>, std::vector<Index>> PolyhedraModel::generateRhombicuboctahedron(float scale) {
    std::vector<Vertex> vertices;
    std::vector<Index> indices;

    float b = 1.0f + std::sqrt(2.0f);

    float rawVerts[24][3] = {
        {-1, -1,  b}, { 1, -1,  b}, { 1,  1,  b}, {-1,  1,  b},
        {-1, -1, -b}, { 1, -1, -b}, { 1,  1, -b}, {-1,  1, -b},
        {-1,  b, -1}, { 1,  b, -1}, { 1,  b,  1}, {-1,  b,  1},
        {-1, -b, -1}, { 1, -b, -1}, { 1, -b,  1}, {-1, -b,  1},
        {-b, -1, -1}, {-b,  1, -1}, {-b,  1,  1}, {-b, -1,  1},
        { b, -1, -1}, { b,  1, -1}, { b,  1,  1}, { b, -1,  1}
    };

    for (int i = 0; i < 24; ++i) {
        addNormalizedVertex(vertices, rawVerts[i][0], rawVerts[i][1], rawVerts[i][2], scale);
    }

    // 18 Square faces
    Index rawQuads[18][4] = {
        {0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15},
        {16, 17, 18, 19}, {20, 21, 22, 23}, {0, 1, 14, 15}, {2, 3, 11, 10},
        {4, 5, 13, 12}, {6, 7, 8, 9}, {0, 3, 18, 19}, {1, 2, 22, 23},
        {4, 7, 17, 16}, {5, 6, 21, 20}, {8, 11, 18, 17}, {9, 10, 22, 21},
        {12, 15, 19, 16}, {13, 14, 23, 20}
    };

    for (int i = 0; i < 18; ++i) {
        indices.push_back(rawQuads[i][0]);
        indices.push_back(rawQuads[i][1]);
        indices.push_back(rawQuads[i][2]);

        indices.push_back(rawQuads[i][0]);
        indices.push_back(rawQuads[i][2]);
        indices.push_back(rawQuads[i][3]);
    }

    // 8 Triangular faces
    Index rawTris[8][3] = {
        {0, 15, 19}, {1, 14, 23}, {2, 10, 22}, {3, 11, 18},
        {4, 12, 16}, {5, 13, 20}, {6, 9, 21}, {7, 8, 17}
    };

    for (int i = 0; i < 8; ++i) {
        indices.push_back(rawTris[i][0]);
        indices.push_back(rawTris[i][1]);
        indices.push_back(rawTris[i][2]);
    }

    return {vertices, indices};
}

// 4. RHOMBIC TRIACONTAHEDRON (30 rhombic faces, 32 vertices)
std::pair<std::vector<Vertex>, std::vector<Index>> PolyhedraModel::generateRhombicTriacontahedron(float scale) {
    std::vector<Vertex> vertices;
    std::vector<Index> indices;

    float phi = (1.0f + std::sqrt(5.0f)) * 0.5f;
    float invPhi = 1.0f / phi;

    float rawVerts[32][3] = {
        // 12 Icosahedron vertices (0..11)
        {-1,  phi, 0}, { 1,  phi, 0}, {-1, -phi, 0}, { 1, -phi, 0},
        { 0, -1,  phi}, { 0,  1,  phi}, { 0, -1, -phi}, { 0,  1, -phi},
        { phi, 0, -1}, { phi, 0,  1}, {-phi, 0, -1}, {-phi, 0,  1},

        // 20 Dodecahedron vertices (12..31)
        {-1, -1, -1}, { 1, -1, -1}, { 1,  1, -1}, {-1,  1, -1},
        {-1, -1,  1}, { 1, -1,  1}, { 1,  1,  1}, {-1,  1,  1},
        {0, -invPhi, -phi}, {0,  invPhi, -phi}, {0, -invPhi,  phi}, {0,  invPhi,  phi},
        {-invPhi, -phi, 0}, { invPhi, -phi, 0}, {-invPhi,  phi, 0}, { invPhi,  phi, 0},
        {-phi, 0, -invPhi}, { phi, 0, -invPhi}, {-phi, 0,  invPhi}, { phi, 0,  invPhi}
    };

    for (int i = 0; i < 32; ++i) {
        addNormalizedVertex(vertices, rawVerts[i][0], rawVerts[i][1], rawVerts[i][2], scale);
    }

    // 30 Rhombic faces (each defined by 4 vertices)
    Index rawQuads[30][4] = {
        {0, 26, 1, 27}, {0, 26, 15, 28}, {0, 27, 14, 28}, {1, 27, 14, 29},
        {1, 26, 15, 29}, {2, 24, 3, 25}, {2, 24, 12, 28}, {2, 25, 13, 28},
        {3, 25, 13, 29}, {3, 24, 12, 29}, {4, 22, 5, 23}, {4, 22, 16, 24},
        {4, 23, 17, 25}, {5, 23, 17, 27}, {5, 22, 16, 26}, {6, 20, 7, 21},
        {6, 20, 12, 24}, {6, 21, 13, 25}, {7, 21, 13, 27}, {7, 20, 12, 26},
        {8, 29, 9, 31}, {8, 29, 13, 25}, {8, 31, 14, 27}, {9, 31, 14, 26},
        {9, 29, 13, 24}, {10, 28, 11, 30}, {10, 28, 12, 24}, {10, 30, 15, 26},
        {11, 30, 15, 27}, {11, 28, 12, 25}
    };

    for (int i = 0; i < 30; ++i) {
        indices.push_back(rawQuads[i][0]);
        indices.push_back(rawQuads[i][1]);
        indices.push_back(rawQuads[i][2]);

        indices.push_back(rawQuads[i][0]);
        indices.push_back(rawQuads[i][2]);
        indices.push_back(rawQuads[i][3]);
    }

    return {vertices, indices};
}
