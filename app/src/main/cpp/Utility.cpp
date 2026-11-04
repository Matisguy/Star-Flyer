#include "Utility.h"
#include "AndroidOut.h"

#include <GLES3/gl3.h>

#define CHECK_ERROR(e) case e: aout << "GL Error: "#e << std::endl; break;

bool Utility::checkAndLogGlError(bool alwaysLog) {
    GLenum error = glGetError();
    if (error == GL_NO_ERROR) {
        if (alwaysLog) {
            aout << "No GL error" << std::endl;
        }
        return true;
    } else {
        switch (error) {
            CHECK_ERROR(GL_INVALID_ENUM);
            CHECK_ERROR(GL_INVALID_VALUE);
            CHECK_ERROR(GL_INVALID_OPERATION);
            CHECK_ERROR(GL_INVALID_FRAMEBUFFER_OPERATION);
            CHECK_ERROR(GL_OUT_OF_MEMORY);
            default:
                aout << "Unknown GL error: " << error << std::endl;
        }
        return false;
    }
}

float *
Utility::buildOrthographicMatrix(float *outMatrix, float halfHeight, float aspect, float near,
                                 float far) {
    float halfWidth = halfHeight * aspect;

    // column 1
    outMatrix[0] = 1.f / halfWidth;
    outMatrix[1] = 0.f;
    outMatrix[2] = 0.f;
    outMatrix[3] = 0.f;

    // column 2
    outMatrix[4] = 0.f;
    outMatrix[5] = 1.f / halfHeight;
    outMatrix[6] = 0.f;
    outMatrix[7] = 0.f;

    // column 3
    outMatrix[8] = 0.f;
    outMatrix[9] = 0.f;
    outMatrix[10] = -2.f / (far - near);
    outMatrix[11] = -(far + near) / (far - near);

    // column 4
    outMatrix[12] = 0.f;
    outMatrix[13] = 0.f;
    outMatrix[14] = 0.f;
    outMatrix[15] = 1.f;

    return outMatrix;
}

float *Utility::buildPerspectiveMatrix(float *outMatrix, float fovDegrees, float aspect, float zNear, float zFar) {
    float f = 1.0f / tanf(fovDegrees * 0.5f * (3.1415926535f / 180.0f));

    // column 1
    outMatrix[0] = f / aspect;
    outMatrix[1] = 0.0f;
    outMatrix[2] = 0.0f;
    outMatrix[3] = 0.0f;

    // column 2
    outMatrix[4] = 0.0f;
    outMatrix[5] = f;
    outMatrix[6] = 0.0f;
    outMatrix[7] = 0.0f;

    // column 3
    outMatrix[8] = 0.0f;
    outMatrix[9] = 0.0f;
    outMatrix[10] = (zFar + zNear) / (zNear - zFar);
    outMatrix[11] = -1.0f;

    // column 4
    outMatrix[12] = 0.0f;
    outMatrix[13] = 0.0f;
    outMatrix[14] = (2.0f * zFar * zNear) / (zNear - zFar);
    outMatrix[15] = 0.0f;

    return outMatrix;
}

float *Utility::multiplyMatrix4x4(const float *a, const float *b, float *outMatrix) {
    float temp[16];
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            temp[col * 4 + row] =
                a[0 * 4 + row] * b[col * 4 + 0] +
                a[1 * 4 + row] * b[col * 4 + 1] +
                a[2 * 4 + row] * b[col * 4 + 2] +
                a[3 * 4 + row] * b[col * 4 + 3];
        }
    }
    for (int i = 0; i < 16; ++i) {
        outMatrix[i] = temp[i];
    }
    return outMatrix;
}

float *Utility::buildCameraViewMatrix(float *outMatrix, float pitchDeg, float rollDeg, float yawDeg) {
    float radP = pitchDeg * (3.1415926535f / 180.0f);
    float radR = rollDeg  * (3.1415926535f / 180.0f);
    float radY = yawDeg   * (3.1415926535f / 180.0f);

    float cP = cosf(radP), sP = sinf(radP);
    float cR = cosf(radR), sR = sinf(radR);
    float cY = cosf(radY), sY = sinf(radY);

    // Rotation around X (Pitch) in column-major layout
    float rotX[16] = {
        1.0f,  0.0f, 0.0f, 0.0f,
        0.0f,   cP,   sP,  0.0f,
        0.0f,  -sP,   cP,  0.0f,
        0.0f,  0.0f, 0.0f, 1.0f
    };

    // Rotation around Y (Yaw) in column-major layout
    float rotY[16] = {
         cY, 0.0f, -sY, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
         sY, 0.0f,  cY, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    // Rotation around Z (Roll) in column-major layout
    float rotZ[16] = {
         cR,   sR, 0.0f, 0.0f,
        -sR,   cR, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    float temp[16];
    multiplyMatrix4x4(rotX, rotY, temp);
    multiplyMatrix4x4(rotZ, temp, outMatrix);

    return outMatrix;
}

float *Utility::buildIdentityMatrix(float *outMatrix) {
    // column 1
    outMatrix[0] = 1.f;
    outMatrix[1] = 0.f;
    outMatrix[2] = 0.f;
    outMatrix[3] = 0.f;

    // column 2
    outMatrix[4] = 0.f;
    outMatrix[5] = 1.f;
    outMatrix[6] = 0.f;
    outMatrix[7] = 0.f;

    // column 3
    outMatrix[8] = 0.f;
    outMatrix[9] = 0.f;
    outMatrix[10] = 1.f;
    outMatrix[11] = 0.f;

    // column 4
    outMatrix[12] = 0.f;
    outMatrix[13] = 0.f;
    outMatrix[14] = 0.f;
    outMatrix[15] = 1.f;

    return outMatrix;
}

float *Utility::buildMatrixFromVectors(float *outMatrix, float *dirVector, float *bodyVector) {
    float temp = 0.f;
    for (int i = 0; i < 3; ++i) {   //norm get
        temp += dirVector[i] * dirVector[i];
    }
    temp = 1.0f/sqrt(temp);
    for (int i = 0; i < 3; ++i) {   //normalize
        dirVector[i] = dirVector[i] * temp;
    }
    temp = 0.f;
    for (int i = 0; i < 3; ++i) {   //dot product
        temp += dirVector[i] * bodyVector[i];
    }
    for (int i = 0; i < 3; ++i) {   //orthogonalize
        bodyVector[i] -= temp * dirVector[i];
    }
    temp = 0.f;
    for (int i = 0; i < 3; ++i) {   //norm get
        temp += bodyVector[i] * bodyVector[i];
    }
    temp = 1.0f/sqrt(temp);
    for (int i = 0; i < 3; ++i) {   //normalize
        bodyVector[i] = bodyVector[i] * temp;
    }
    //direction vector
    outMatrix[0] = dirVector[0];
    outMatrix[1] = dirVector[1];
    outMatrix[2] = dirVector[2];
    outMatrix[3] = 0.f;
    //body vector
    outMatrix[4] = bodyVector[0];
    outMatrix[5] = bodyVector[1];
    outMatrix[6] = bodyVector[2];
    outMatrix[7] = 0.f;
    //cross product
    outMatrix[8] = dirVector[1] * bodyVector[2] - dirVector[2] * bodyVector[1];
    outMatrix[9] = dirVector[2] * bodyVector[0] - dirVector[0] * bodyVector[2];
    outMatrix[10] = dirVector[0] * bodyVector[1] - dirVector[1] * bodyVector[0];
    outMatrix[11] = 0.f;
    //orthonormal matrix
    outMatrix[12] = 0.f;
    outMatrix[13] = 0.f;
    outMatrix[14] = 0.f;
    outMatrix[15] = 1.f;

    return outMatrix;
}

float *Utility::normalizeVector(float *vector) {
    float temp = 0.f;
    for (int i = 0; i < 3; ++i) {   //norm get
        temp += vector[i] * vector[i];
    }
    temp = 1.0f/sqrt(temp);
    for (int i = 0; i < 3; ++i) {   //normalize
        vector[i] = vector[i] * temp;
    }
    return vector;
}