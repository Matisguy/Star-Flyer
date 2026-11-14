#ifndef STARFLYER_POLYHEDRA_MODEL_H
#define STARFLYER_POLYHEDRA_MODEL_H

#include <vector>
#include <utility>
#include "Model.h"

class PolyhedraModel {
public:
    static std::pair<std::vector<Vertex>, std::vector<Index>> generateIcosahedron(float scale = 1.0f);
    static std::pair<std::vector<Vertex>, std::vector<Index>> generateRhombicDodecahedron(float scale = 1.0f);
    static std::pair<std::vector<Vertex>, std::vector<Index>> generateRhombicuboctahedron(float scale = 1.0f);
    static std::pair<std::vector<Vertex>, std::vector<Index>> generateRhombicTriacontahedron(float scale = 1.0f);
};

#endif // STARFLYER_POLYHEDRA_MODEL_H
