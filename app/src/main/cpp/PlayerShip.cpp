#include "PlayerShip.h"
#include "Utility.h"
#include <cmath>

PlayerShip::PlayerShip() {
    rigidBody_ = std::make_unique<RigidBody>(
        1.0f,
        1.0f,
        Vec3(0.0f, 0.0f, 0.0f),
        Vec3(0.0f, 0.8f, 1.0f) // Cyan hull accent
    );
    rigidBody_->scale = 1.0f;
    initialized_ = false;
}

void PlayerShip::calibrate(const float viewSensorMatrix[16]) {
    // 1. V_calib_transpose = V_sensor^T
    float vCalibT[16];
    vCalibT[0] = viewSensorMatrix[0]; vCalibT[1] = viewSensorMatrix[4]; vCalibT[2] = viewSensorMatrix[8];  vCalibT[3] = 0.0f;
    vCalibT[4] = viewSensorMatrix[1]; vCalibT[5] = viewSensorMatrix[5]; vCalibT[6] = viewSensorMatrix[9];  vCalibT[7] = 0.0f;
    vCalibT[8] = viewSensorMatrix[2]; vCalibT[9] = viewSensorMatrix[6]; vCalibT[10] = viewSensorMatrix[10]; vCalibT[11] = 0.0f;
    vCalibT[12] = 0.0f; vCalibT[13] = 0.0f; vCalibT[14] = 0.0f; vCalibT[15] = 1.0f;

    // 2. R_ship_transpose = R_ship^T
    float shipMatrix[16];
    rigidBody_->orientation.toMatrix(shipMatrix);

    float rShipT[16];
    rShipT[0] = shipMatrix[0]; rShipT[1] = shipMatrix[4]; rShipT[2] = shipMatrix[8];  rShipT[3] = 0.0f;
    rShipT[4] = shipMatrix[1]; rShipT[5] = shipMatrix[5]; rShipT[6] = shipMatrix[9];  rShipT[7] = 0.0f;
    rShipT[8] = shipMatrix[2]; rShipT[9] = shipMatrix[6]; rShipT[10] = shipMatrix[10]; rShipT[11] = 0.0f;
    rShipT[12] = 0.0f; rShipT[13] = 0.0f; rShipT[14] = 0.0f; rShipT[15] = 1.0f;

    // 3. Calibration Matrix C = V_calib^T * R_ship^T
    Utility::multiplyMatrix4x4(vCalibT, rShipT, worldOrienter_);
    for (int i = 0; i < 16; ++i) {
        worldOrienter_[i] = vCalibT[i];
    }
    // Save calibration phone matrix
    for (int i = 0; i < 16; ++i) {
        calibSensorMatrix_[i] = viewSensorMatrix[i];
    }
}

void PlayerShip::update(float dt, const Vec3 &cameraVector, float phonePitch, const float viewSensorMatrix[16]) {
    if (!initialized_) {
        calibrate(viewSensorMatrix);
        initialized_ = true;
    }

    if (gGameSettings.calibrationRequested) {
        calibrate(viewSensorMatrix);
        gGameSettings.calibrationRequested = false;
    }

    // Relative phone tilt matrix: DeltaV = V_sensor * V_calib^T
    float vCalibT[16];
    vCalibT[0] = calibSensorMatrix_[0]; vCalibT[1] = calibSensorMatrix_[4]; vCalibT[2] = calibSensorMatrix_[8];  vCalibT[3] = 0.0f;
    vCalibT[4] = calibSensorMatrix_[1]; vCalibT[5] = calibSensorMatrix_[5]; vCalibT[6] = calibSensorMatrix_[9];  vCalibT[7] = 0.0f;
    vCalibT[8] = calibSensorMatrix_[2]; vCalibT[9] = calibSensorMatrix_[6]; vCalibT[10] = calibSensorMatrix_[10]; vCalibT[11] = 0.0f;
    vCalibT[12] = 0.0f; vCalibT[13] = 0.0f; vCalibT[14] = 0.0f; vCalibT[15] = 1.0f;

    float deltaV[16];
    Utility::multiplyMatrix4x4(viewSensorMatrix, vCalibT, deltaV);

    // Tilt look direction in local camera space
    float dx = deltaV[8];
    float dy = deltaV[9];

    // Local steering torque: (dy * sensitivity, -dx * sensitivity, 0)
    Vec3 localTorque(
        dy * (gGameSettings.turnSensitivity * handlingAgility),
        -dx * (gGameSettings.turnSensitivity * handlingAgility),
        0.0f
    );

    // Transform local torque into world space using ship's orientation matrix
    float shipMatrix[16];
    rigidBody_->orientation.toMatrix(shipMatrix);

    Vec3 worldTorque(
        shipMatrix[0] * localTorque.x + shipMatrix[4] * localTorque.y + shipMatrix[8]  * localTorque.z,
        shipMatrix[1] * localTorque.x + shipMatrix[5] * localTorque.y + shipMatrix[9]  * localTorque.z,
        shipMatrix[2] * localTorque.x + shipMatrix[6] * localTorque.y + shipMatrix[10] * localTorque.z
    );

    rigidBody_->angularVelocity = worldTorque;

    // Set ship forward flight velocity (along ship's -Z look axis)
    float currentSpeed = shipSpeed * enginePower;
    Vec3 shipForward(-shipMatrix[8], -shipMatrix[9], -shipMatrix[10]);
    rigidBody_->velocity = shipForward * currentSpeed;

    // Update ship physics state
    rigidBody_->update(dt);
}
