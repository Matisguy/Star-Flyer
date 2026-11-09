#ifndef STARFLYER_ORIENTATION_TRACKER_H
#define STARFLYER_ORIENTATION_TRACKER_H

#include "SensorData.h"

class OrientationTracker {
public:
    OrientationTracker();

    /**
     * Updates orientation state using gyro integration + accelerometer/magnetometer fusion
     * @param dt Elapsed time in seconds since last frame
     */
    void update(float dt);

    /**
     * Computes 4x4 column-major camera view matrix mapped for landscape flight control
     * @param outMatrix Pointer to 16-element float array
     */
    void getViewMatrix(float *outMatrix) const;

    /**
     * Gets current smoothed gravity vector
     */
    void getGravity(float out[3]) const;

    void getDeviceOrientation(float out[4]) const;

private:
    float grav_[3]  = {0.0f, 9.81f, 0.0f};
    float orient_[4] = {1.0f, 0.0f, 0.0f, 0.0f};
    bool initialized_ = false;
};

#endif // STARFLYER_ORIENTATION_TRACKER_H
