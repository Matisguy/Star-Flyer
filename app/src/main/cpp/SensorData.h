#ifndef STARFLYER_SENSOR_DATA_H
#define STARFLYER_SENSOR_DATA_H

#include <mutex>

struct SensorData {
    float accel[3] = {0.0f, 0.0f, 0.0f};
    float gyro[3]  = {0.0f, 0.0f, 0.0f};
    float mag[3]   = {0.0f, 0.0f, 0.0f};
    float pitch    = 0.0f; // in degrees
    float roll     = 0.0f; // in degrees
    float yaw      = 0.0f; // in degrees
};

extern SensorData gSensorData;
extern std::mutex gSensorMutex;

#endif // STARFLYER_SENSOR_DATA_H
