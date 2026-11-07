#ifndef STARFLYER_TEAPOT_MODEL_H
#define STARFLYER_TEAPOT_MODEL_H

#include <vector>
#include <utility>
#include "Model.h"

class TeapotModel {
public:
    /**
     * Generates a 3D mesh (vertices and indices) for the Utah Teapot
     * @param scale Overall size multiplier
     */
    static std::pair<std::vector<Vertex>, std::vector<Index>> generateMesh(float scale = 1.0f);
};

#endif // STARFLYER_TEAPOT_MODEL_H
