#ifndef STARFLYER_SENSOR_DATA_H
#define STARFLYER_SENSOR_DATA_H

#include <mutex>

struct SensorData {
    float grav[3] = {0.0f, 0.0f, 0.0f};
    float gyro[3]  = {0.0f, 0.0f, 0.0f};
    float orientation[4]   = {0.0f, 0.0f, 0.0f, 1.0f};
};

extern SensorData gSensorData;
extern std::mutex gSensorMutex;

#endif // STARFLYER_SENSOR_DATA_H
