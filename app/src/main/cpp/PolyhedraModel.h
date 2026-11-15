#ifndef STARFLYER_POLYHEDRA_MODEL_H
#define STARFLYER_POLYHEDRA_MODEL_H

#include <vector>
#include <utility>
#include "Model.h"

class PolyhedraModel {
public:
    static std::pair<std::vector<Vertex>, std::vector<Index>> generateIcosahedron(float scale = 1.0f);
    static std::pair<std::vector<Vertex>, std::vector<Index>> generateRhombicDodecahedron(float scale = 1.0f);
    static std::pair<std::vector<Vertex>, std::vector<Index>> generateCuboctahedron(float scale = 1.0f);
    static std::pair<std::vector<Vertex>, std::vector<Index>> generateRhombicuboctahedron(float scale = 1.0f);
    static std::pair<std::vector<Vertex>, std::vector<Index>> generateRhombicTriacontahedron(float scale = 1.0f);
    static std::pair<std::vector<Vertex>, std::vector<Index>> generateReticle1(float scale = 1.0f);
    static std::pair<std::vector<Vertex>, std::vector<Index>> generateReticle2(float scale = 1.0f);
    static std::pair<std::vector<Vertex>, std::vector<Index>> generateUFOskirt(float inScale = 0.5f, float outScale = 0.5f, float thk = 0.2f);
};

#endif // STARFLYER_POLYHEDRA_MODEL_H
