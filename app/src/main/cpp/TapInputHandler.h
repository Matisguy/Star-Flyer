#ifndef STARFLYER_TAP_INPUT_HANDLER_H
#define STARFLYER_TAP_INPUT_HANDLER_H

#include <SDL3/SDL.h>
#include <memory>
#include <string>
#include <vector>
#include "RigidBody.h"
#include "GameSettings.h"

struct HitmarkerEffect {
    float x = 0.0f;
    float y = 0.0f;
    float timeRemaining = 0.0f; // 0.05 seconds duration
};

struct FloatingMessage {
    Vec3 position;
    std::string text;
    float timeRemaining = 2.0f; // 2.0 seconds duration
};

class TapInputHandler {
public:
    TapInputHandler();

    void handleEvent(const SDL_Event &event, int screenWidth, int screenHeight);
    void update(float dt, int screenWidth, int screenHeight, const float viewMatrix[16], const float projectionMatrix[16], std::vector<std::unique_ptr<RigidBody>> &teapots);

    bool isTapActive() const { return isHeld_; }
    void getTapPosition(float &x, float &y) const { x = tapX_; y = tapY_; }
    float getHoldDuration() const { return holdDuration_; }

    const HitmarkerEffect& getHitmarker() const { return hitmarker_; }
    const std::vector<FloatingMessage>& getFloatingMessages() const { return floatingMessages_; }

private:
    bool isHeld_ = false;
    float tapX_ = 0.0f;
    float tapY_ = 0.0f;
    float holdDuration_ = 0.0f;

    RigidBody* pointedObject_ = nullptr;
    HitmarkerEffect hitmarker_;
    std::vector<FloatingMessage> floatingMessages_;
};

#endif // STARFLYER_TAP_INPUT_HANDLER_H
