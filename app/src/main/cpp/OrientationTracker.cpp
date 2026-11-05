#include "OrientationTracker.h"
#include "Utility.h"
#include <cmath>

OrientationTracker::OrientationTracker() {
    grav_[0] = 0.0f; grav_[1] = 9.81f; grav_[2] = 0.0f;
    north_[0] = 0.0f; north_[1] = 0.0f; north_[2] = 1.0f;
}

void OrientationTracker::update(float dt) {
    float accel[3], gyro[3], mag[3];
    {
        std::lock_guard<std::mutex> lock(gSensorMutex);
        for (int i = 0; i < 3; ++i) {
            accel[i] = gSensorData.accel[i];
            gyro[i]  = gSensorData.gyro[i];
            mag[i]   = gSensorData.mag[i];
        }
    }

    if (!initialized_) {
        // Initialize filtered vectors with raw sensor readings
        for (int i = 0; i < 3; ++i) {
            grav_[i] = accel[i];
            north_[i] = mag[i];
        }
        initialized_ = true;
    }

    // 1. Gyroscope first-order integration: dV/dt = V x w
    // dGrav/dt = grav x gyro
    float dGrav[3] = {
        grav_[1] * gyro[2] - grav_[2] * gyro[1],
        grav_[2] * gyro[0] - grav_[0] * gyro[2],
        grav_[0] * gyro[1] - grav_[1] * gyro[0]
    };

    // dNorth/dt = north x gyro
    float dNorth[3] = {
        north_[1] * gyro[2] - north_[2] * gyro[1],
        north_[2] * gyro[0] - north_[0] * gyro[2],
        north_[0] * gyro[1] - north_[1] * gyro[0]
    };

    // Integrate gyro delta
    for (int i = 0; i < 3; ++i) {
        grav_[i]  += dGrav[i]  * dt;
        north_[i] += dNorth[i] * dt;
    }

    // 2. Complementary Filter: blend gyro-integrated estimate with raw accel/mag
    // Smoothness factor alpha (time-invariant decay)
    float alpha = std::exp(-dt * 0.2f);
    float dataWeight = 1.0f - alpha;

    for (int i = 0; i < 3; ++i) {
        grav_[i]  = grav_[i]  * alpha + accel[i] * dataWeight;
        north_[i] = north_[i] * alpha + mag[i]   * dataWeight;
    }

    // 3. Normalize vectors
    Utility::normalizeVector(grav_);
    Utility::normalizeVector(north_);
}

void OrientationTracker::getViewMatrix(float *outMatrix) const {
    // In Landscape mode (phone held horizontally for flight sim):
    // Up vector: -grav (since gravity points down towards ground)
    float up[3] = { -grav_[0], -grav_[1], -grav_[2] };
    Utility::normalizeVector(up);

    // Right vector: Up x North
    float right[3] = {
        up[1] * north_[2] - up[2] * north_[1],
        up[2] * north_[0] - up[0] * north_[2],
        up[0] * north_[1] - up[1] * north_[0]
    };
    Utility::normalizeVector(right);

    // Forward vector (Look direction): Right x Up
    float fwd[3] = {
        right[1] * up[2] - right[2] * up[1],
        right[2] * up[0] - right[0] * up[2],
        right[0] * up[1] - right[1] * up[0]
    };
    Utility::normalizeVector(fwd);

    // Build 4x4 column-major View Matrix
    outMatrix[0] = right[0];
    outMatrix[1] = right[1];
    outMatrix[2] = right[2];
    outMatrix[3] = 0.0f;

    outMatrix[4] = up[0];
    outMatrix[5] = up[1];
    outMatrix[6] = up[2];
    outMatrix[7] = 0.0f;

    outMatrix[8] = fwd[0];
    outMatrix[9] = fwd[1];
    outMatrix[10] = fwd[2];
    outMatrix[11] = 0.0f;

    outMatrix[12] = 0.0f;
    outMatrix[13] = 0.0f;
    outMatrix[14] = 0.0f;
    outMatrix[15] = 1.0f;
}

void OrientationTracker::getGravity(float out[3]) const {
    out[0] = grav_[0];
    out[1] = grav_[1];
    out[2] = grav_[2];
}

void OrientationTracker::getNorth(float out[3]) const {
    out[0] = north_[0];
    out[1] = north_[1];
    out[2] = north_[2];
}
