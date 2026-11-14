#ifndef STAR_FLYER_PLAYERSHIP_H
#define STAR_FLYER_PLAYERSHIP_H

#include "RigidBody.h"
#include "GameSettings.h"
#include <memory>

class PlayerShip {
public:
    PlayerShip();

    void update(float dt, const Vec3 &cameraVector, const float viewMatrix[16]);
    void calibrate(const float viewMatrix[16]);

    RigidBody& getRigidBody() { return *rigidBody_; }
    const RigidBody& getRigidBody() const { return *rigidBody_; }

    const float* getWorldOrienter() const { return worldOrienter_; }

    // Workshop Customization Properties
    int shipType = 0;
    float shipSpeed = 6.0f;
    float enginePower = 1.0f;
    float handlingAgility = 1.0f;
    float baseDamage = 12.0f;
    float chargeDamage = 30.0f;
    float chargeTime = 1.0f;
    float maxShields = 50.0f;
    float currentShields = 50.0f;
    int maxShieldCount = 2;
    int currentShieldCount = 2;
    float maxHealth = 100.0f;
    float currentHealth = 100.0f;

private:
    std::unique_ptr<RigidBody> rigidBody_;
    float calibSensorMatrix_[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    float worldOrienter_[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    bool initialized_ = false;
};

#endif // STAR_FLYER_PLAYERSHIP_H
