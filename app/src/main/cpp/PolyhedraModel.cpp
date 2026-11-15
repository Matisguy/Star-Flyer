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

std::pair<std::vector<Vertex>, std::vector<Index>> PolyhedraModel::generateCuboctahedron(float scale) {
    std::vector<Vertex> vertices;
    std::vector<Index> indices;

    float rawVerts[12][3] = {
            {1,0, 1},{ 0,1, 1},{-1, 0, 1},{0,-1, 1},
            {1,1, 0},{-1,1, 0},{-1,-1, 0},{1,-1, 0},
            {1,0,-1},{ 0,1,-1},{-1, 0,-1},{0,-1,-1}
    };

    for (int i = 0; i < 12; ++i) {
        addNormalizedVertex(vertices, rawVerts[i][0], rawVerts[i][1], rawVerts[i][2], scale);
    }

    // 14 faces (6 of which are squares)
    Index rawIndices[20][3] = {
            {0,1,4},{1,2,5},{2,3,6},{3,0,7},    //triangles
            {8,9,4},{9,10,5},{10,11,6},{11,8,7},
            {0,1,2},{0,3,2},{8,9,10},{8,11,10},   // Z squares
            {1,4,9},{1,5,9},{3,6,11},{3,7,11},  // Y squares
            {4,0,7},{4,8,7},{5,2,6},{5,10,6},   // X squares
    };

    for (int i = 0; i < 12; ++i) {
        indices.push_back(rawIndices[i][0]);
        indices.push_back(rawIndices[i][1]);
        indices.push_back(rawIndices[i][2]);
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

std::pair<std::vector<Vertex>, std::vector<Index>> PolyhedraModel::generateReticle1(float scale) {
    std::vector<Vertex> vertices;
    std::vector<Index> indices;

    float reach = (1.0f + std::sqrt(2.0f));
    float thk = 0.1f;

    float rawVerts[32][3] = {
            {1,  reach,   0},  {1.0f + thk, reach*(1.0f+thk), thk}, {1.0f + 2.0f*thk, reach*(1.0f+2.0f*thk), 0}, {1.0f + thk, reach*(1.0f+thk), -thk},
            {reach,  1,   0},  {reach*(1.0f+thk), 1.0f + thk, thk}, {reach*(1.0f+2.0f*thk), 1.0f + 2.0f*thk, 0}, {reach*(1.0f+thk), 1.0f + thk, -thk},
            {reach, -1,   0},  {reach*(1.0f+thk), -1.0f- thk, thk}, {reach*(1.0f+2.0f*thk), -1.0f- 2.0f*thk, 0}, {reach*(1.0f+thk),-1.0f - thk, -thk},
            {1,  -reach,  0},  {1.0f + thk,-reach*(1.0f+thk), thk}, {1.0f + 2.0f*thk,-reach*(1.0f+2.0f*thk), 0}, {1.0f + thk,-reach*(1.0f+thk), -thk},
            {-1, -reach,  0},  {-1.0f -thk,-reach*(1.0f+thk), thk}, {-1.0f -2.0f*thk,-reach*(1.0f+2.0f*thk), 0}, {-1.0f- thk,-reach*(1.0f+thk), -thk},
            {-reach, -1,  0},  {-reach*(1.0f+thk),-1.0f -thk, thk}, {-reach*(1.0f+2.0f*thk),-1.0f -2.0f*thk, 0}, {-reach*(1.0f+thk),-1.0f -thk, -thk},
            {-reach,  1,  0},  {-reach*(1.0f+thk),1.0f + thk, thk}, {-reach*(1.0f+2.0f*thk),1.0f + 2.0f*thk, 0}, {-reach*(1.0f+thk),1.0f + thk, -thk},
            {-1,  reach,  0},  {-1.0f -thk, reach*(1.0f+thk), thk}, {-1.0f - 2.0f*thk, reach*(1.0f+2.0f*thk), 0}, {-1.0f -thk,reach*(1.0f+thk), -thk},
    };

    for (int i = 0; i < 8; ++i) {
        addNormalizedVertex(vertices, rawVerts[4*i+0][0], rawVerts[4*i+0][1], rawVerts[4*i+0][2], scale);
        addNormalizedVertex(vertices, rawVerts[4*i+1][0], rawVerts[4*i+1][1], rawVerts[4*i+1][2], scale * (1.0f+thk));
        addNormalizedVertex(vertices, rawVerts[4*i+2][0], rawVerts[4*i+2][1], rawVerts[4*i+2][2], scale * (1.0f+thk+thk));
        addNormalizedVertex(vertices, rawVerts[4*i+3][0], rawVerts[4*i+3][1], rawVerts[4*i+3][2], scale * (1.0f+thk));
    }

    Index rawIndices[28][4] = {
            { 0, 1, 5, 4}, { 1, 2, 6, 5}, { 2, 3, 7, 6}, { 3,0, 4, 7},
            { 4, 5, 9, 8}, { 5, 6,10, 9}, { 6, 7,11,10}, { 7,4, 8,11},
            { 8, 9,13,12}, { 9,10,14,13},{10,11,15,14}, {11,8,12,15},
            {0,1,2,3}, {12,13,14,15}, {16,17,18,19}, {28,29,30,31}, //row of end caps
            {16,17,21,20},{17,18,22,21},{18,19,23,22},{19,16,20,23},
            {20,21,25,24},{21,22,26,25},{22,23,27,26},{23,20,24,27},
            {24,25,29,28},{25,26,30,29},{26,27,31,30},{27,24,28,31}
    };

    for (int i = 0; i < 28; ++i) {
        indices.push_back(rawIndices[i][0]);
        indices.push_back(rawIndices[i][1]);
        indices.push_back(rawIndices[i][2]);

        indices.push_back(rawIndices[i][0]);
        indices.push_back(rawIndices[i][2]);
        indices.push_back(rawIndices[i][3]);
    }

    return {vertices, indices};
}

std::pair<std::vector<Vertex>, std::vector<Index>> PolyhedraModel::generateReticle2(float scale) {
    std::vector<Vertex> vertices;
    std::vector<Index> indices;

    float rawVerts[16][3]= {
            { 1, 1, 1}, { 2.2, 1.8, 1.8}, { 1.8, 2.2, 1.8},{ 1.8, 1.8, 2.2},
            { 2.2,-1.8,-1.8}, { 1,-1,-1}, { 1.8,-1.8,-2.2},{ 1.8,-2.2,-1.8},
            {-1.8, 2.2,-1.8}, {-1.8, 1.8,-2.2}, {-1, 1,-1},{-2.2, 1.8,-1.8},
            {-1.8,-1.8, 2.2},{-1.8,-2.2, 1.8},{-2.2,-1.8, 1.8},{-1,-1, 1}
    };

    for (int i = 0; i < 16; ++i) {
        if (i % 5 == 0) {
            addNormalizedVertex(vertices, rawVerts[i][0], rawVerts[i][1], rawVerts[i][2], 0.5f*scale);
        } else {
            addNormalizedVertex(vertices, rawVerts[i][0], rawVerts[i][1], rawVerts[i][2], scale);
        }
    }

    Index rawIndices[16][3] = {
            {0,1,2},{1,2,3},{2,3,0},{3,0,1},
            {4,5,6},{5,6,7},{6,7,4},{7,4,5},
            {8,9,10},{9,10,11},{10,11,8},{11,8,9},
            {12,13,14},{13,14,15},{14,15,12},{15,12,13}
    };

    for (int i = 0; i < 16; ++i) {
        indices.push_back(rawIndices[i][0]);
        indices.push_back(rawIndices[i][1]);
        indices.push_back(rawIndices[i][2]);
    }

    return {vertices, indices};
}

std::pair<std::vector<Vertex>, std::vector<Index>> PolyhedraModel::generateUFOskirt(float inScale, float outScale, float thk) {
    std::vector<Vertex> vertices;
    std::vector<Index> indices;

    float c16th = cosf(0.0625f*3.1415926535f);
    float c8th = cosf(0.125f*3.1415926535f);
    float c16th3 = cosf(0.1875f*3.1415926535f);
    float roothalf = sqrt(0.5);
    float s16th3 = sinf(0.1875f*3.1415926535f);
    float s8th = sinf(0.125f*3.1415926535f);
    float s16th = sinf(0.0625f*3.1415926535f);

    float rawVerts[48][3]= {
            //Outer ring
            {c16th,s16th,0},{c16th3,s16th3,0},{s16th3,c16th3,0},{s16th,c16th,0},
            {-s16th,c16th,0},{-s16th3,c16th3,0},{-c16th3,s16th3,0},{-c16th,s16th,0},
            {-c16th,-s16th,0},{-c16th3,-s16th3,0},{-s16th3,-c16th3,0},{-s16th,-c16th,0},
            {-s16th,c16th,0},{-s16th3,c16th3,0},{-c16th3,s16th3,0},{-c16th,s16th,0},

            //Upper Inner Ring
            {1,0,thk},{c8th,s8th,thk},{roothalf,roothalf,thk},{s8th,c8th,thk},
            {0,1,thk},{-s8th,c8th,thk},{-roothalf,roothalf,thk},{-c8th,s8th,thk},
            {-1,0,thk},{-c8th,-s8th,thk},{-roothalf,-roothalf,thk},{-s8th,-c8th,thk},
            {0,-1,thk},{s8th,-c8th,thk},{roothalf,-roothalf,thk},{c8th,-s8th,thk},

            //Lower Inner Ring
            {1,0,-thk},{c8th,s8th,-thk},{roothalf,roothalf,-thk},{s8th,c8th,-thk},
            {0,1,-thk},{-s8th,c8th,-thk},{-roothalf,roothalf,-thk},{-c8th,s8th,-thk},
            {-1,0,-thk},{-c8th,-s8th,-thk},{-roothalf,-roothalf,-thk},{-s8th,-c8th,-thk},
            {0,-1,-thk},{s8th,-c8th,-thk},{roothalf,-roothalf,-thk},{c8th,-s8th,-thk},
    };

    for (int i = 0; i < 16; ++i) {
        vertices.emplace_back(Vector3{rawVerts[i+16][0] * inScale, rawVerts[i+16][1] * inScale, rawVerts[i+16][2] * inScale}, Vector2{(rawVerts[i+16][0] + 0.5f)*0.25f, (rawVerts[i+16][1] + 0.5f)*0.25f});
        vertices.emplace_back(Vector3{rawVerts[i][0] * outScale, rawVerts[i][1] * outScale, rawVerts[i][2] * outScale}, Vector2{(rawVerts[i][0] + 0.5f)*0.5f, (rawVerts[i][1] + 0.5f)*0.5f});
        vertices.emplace_back(Vector3{rawVerts[i+32][0] * inScale, rawVerts[i+32][1] * inScale, rawVerts[i+32][2] * inScale}, Vector2{(rawVerts[i+32][0] + 0.5f)*0.75f, (rawVerts[i+32][1] + 0.5f)*0.75f});
    }


    Index rawIndices[64][3] = {
            //Upper Half
             {16,0,17},{0,17,1},{17,1,18},{1,18,2},{18,2,19},{2,19,3},{19,3,20},{3,20,4},
             {20,4,21},{4,21,5},{21,5,22},{5,22,6},{22,6,23},{6,23,7},{23,7,24},{7,24,8},
             {24,8,25},{8,25,9},{25,9,26},{9,26,10},{26,10,27},{10,27,11},{27,11,28},{11,28,12},
             {28,12,29},{12,29,13},{29,13,30},{13,30,14},{30,14,31},{14,31,15},{31,15,16},{15,16,0},
             //Lower Half
             {32,0,33},{0,33,1},{33,1,34},{1,34,2},{34,2,35},{2,35,3},{35,3,36},{3,36,4},
             {36,4,37},{4,37,5},{37,5,38},{5,38,6},{38,6,39},{6,39,7},{39,7,40},{7,40,8},
             {40,8,41},{8,41,9},{41,9,42},{9,42,10},{42,10,43},{10,43,11},{43,11,44},{11,44,12},
             {44,12,45},{12,45,13},{45,13,46},{13,46,14},{46,14,47},{14,47,15},{47,15,32},{15,32,0}
    };

    for (int i = 0; i < 64; ++i) {
        indices.push_back(rawIndices[i][0]);
        indices.push_back(rawIndices[i][1]);
        indices.push_back(rawIndices[i][2]);
    }

    return {vertices, indices};
}