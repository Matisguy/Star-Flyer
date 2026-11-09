#ifndef ANDROIDGLINVESTIGATIONS_UTILITY_H
#define ANDROIDGLINVESTIGATIONS_UTILITY_H

#include <cassert>

class Utility {
public:
    static bool checkAndLogGlError(bool alwaysLog = false);

    static inline void assertGlError() { assert(checkAndLogGlError()); }

    /**
     * Generates an orthographic projection matrix given the half height, aspect ratio, near, and far
     * planes
     *
     * @param outMatrix the matrix to write into
     * @param halfHeight half of the height of the screen
     * @param aspect the width of the screen divided by the height
     * @param near the distance of the near plane
     * @param far the distance of the far plane
     * @return the generated matrix, this will be the same as @a outMatrix so you can chain calls
     *     together if needed
     */
    static float *buildOrthographicMatrix(
            float *outMatrix,
            float halfHeight,
            float aspect,
            float near,
            float far);

    /**
     * Generates a 3D perspective projection matrix given FOV in degrees, aspect ratio, near and far planes
     */
    static float *buildPerspectiveMatrix(
            float *outMatrix,
            float fovDegrees,
            float aspect,
            float zNear,
            float zFar);

    /**
     * Generates a camera view matrix based on phone orientation (pitch, roll, yaw in degrees)
     */
    static float *buildCameraViewMatrix(
            float *outMatrix,
            float pitchDeg,
            float rollDeg,
            float yawDeg);

    static float *buildMatrixFromVectors(
            float *outMatrix,
            float *dirVector,
            float *bodyVector);

    /**
     * Multiplies two 4x4 column-major matrices (out = matA * matB)
     */
    static float *multiplyMatrix4x4(
            const float *matA,
            const float *matB,
            float *outMatrix);

    static float *buildIdentityMatrix(float *outMatrix);

    static float *normalizeVector(float *vector, int length);
};

#endif //ANDROIDGLINVESTIGATIONS_UTILITY_H