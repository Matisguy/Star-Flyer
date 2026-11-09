#include "OrientationTracker.h"
#include "Utility.h"
#include "RigidBody.h"
#include <cmath>

OrientationTracker::OrientationTracker() {
    grav_[0] = 0.0f; grav_[1] = 9.81f; grav_[2] = 0.0f;
    orient_[0] = 1.0f; orient_[1] = 0.0f; orient_[2] = 0.0f; orient_[3] = 0.0f;
}

void OrientationTracker::update(float dt) {
    float gravity[3], gyro[3], orienter[4];
    {
        std::lock_guard<std::mutex> lock(gSensorMutex);
        for (int i = 0; i < 3; ++i) {
            gravity[i] = gSensorData.grav[i];
            gyro[i]  = gSensorData.gyro[i];
            orient_[i+1] = gSensorData.orientation[i];
        }
        orient_[0] = gSensorData.orientation[3];
    }
}

void OrientationTracker::getViewMatrix(float *outMatrix) const {
    float w = orient_[0];
    float x = orient_[2];
    float y = -orient_[1];
    float z = -orient_[3];

    float xx = x*x, yy = y*y, zz = z*z;
    float xy = x*y, xz = x*z, yz = y*z;
    float wx = w*x, wy = w*y, wz = w*z;

    // Column 0
    outMatrix[0] = (1.0f - 2.0f * (yy + zz));
    outMatrix[1] = (2.0f * (xy + wz));
    outMatrix[2] = (2.0f * (xz - wy));
    outMatrix[3] = 0.0f;

    // Column 1
    outMatrix[4] = 2.0f * (xy - wz);
    outMatrix[5] = 1.0f - 2.0f * (xx + zz);
    outMatrix[6] = 2.0f * (yz + wx);
    outMatrix[7] = 0.0f;

    // Column 2
    outMatrix[8] = 2.0f * (xz + wy);
    outMatrix[9] = 2.0f * (yz - wx);
    outMatrix[10] = 1.0f - 2.0f * (xx + yy);
    outMatrix[11] = 0.0f;

    // Column 3
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

void OrientationTracker::getDeviceOrientation(float out[4]) const {
    out[0] = orient_[0];
    out[1] = orient_[1];
    out[2] = orient_[2];
    out[3] = orient_[3];
}