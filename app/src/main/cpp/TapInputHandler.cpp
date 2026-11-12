#include "TapInputHandler.h"
#include "Utility.h"
#include <algorithm>
#include <cmath>

TapInputHandler::TapInputHandler() {
    isHeld_ = false;
    tapX_ = 0.0f;
    tapY_ = 0.0f;
    holdDuration_ = 0.0f;
    pointedObject_ = nullptr;
}

#if defined(__ANDROID__)
extern "C" void triggerAndroidBanner(const char* message);
#endif

void TapInputHandler::handleEvent(const SDL_Event &event, int screenWidth, int screenHeight) {
    if (gGameSettings.gameState != GameState::PLAYING) {
        return;
    }

    float x = 0.0f, y = 0.0f;
    bool isDown = false;
    bool isUp = false;
    bool isMotion = false;

    if (event.type == SDL_EVENT_FINGER_DOWN) {
        x = event.tfinger.x * screenWidth;
        y = event.tfinger.y * screenHeight;
        isDown = true;
    } else if (event.type == SDL_EVENT_FINGER_MOTION) {
        x = event.tfinger.x * screenWidth;
        y = event.tfinger.y * screenHeight;
        isMotion = true;
    } else if (event.type == SDL_EVENT_FINGER_UP) {
        x = event.tfinger.x * screenWidth;
        y = event.tfinger.y * screenHeight;
        isUp = true;
    } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        x = event.button.x;
        y = event.button.y;
        isDown = true;
    } else if (event.type == SDL_EVENT_MOUSE_MOTION && (event.motion.state & SDL_BUTTON_LMASK)) {
        x = event.motion.x;
        y = event.motion.y;
        isMotion = true;
    } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
        x = event.button.x;
        y = event.button.y;
        isUp = true;
    }

    // Check HUD Exclusion Zone (Pause button top-left, Calibrate button top-right)
    if (isDown) {
        bool inPauseButton = (x < 220.0f && y < 120.0f);
        bool inCalibButton = (x > (screenWidth - 240.0f) && y < 120.0f);
        if (inPauseButton || inCalibButton) {
            return;
        }

        isHeld_ = true;
        tapX_ = x;
        tapY_ = y;
        holdDuration_ = 0.0f;
    } else if (isMotion && isHeld_) {
        tapX_ = x;
        tapY_ = y;
    } else if (isUp && isHeld_) {
        // Trigger Hitmarker at release point
        hitmarker_.x = tapX_;
        hitmarker_.y = tapY_;
        hitmarker_.timeRemaining = 0.05f; // 0.05 seconds

        // Send shotAt and chargedShot signals to pointed teapot
        if (pointedObject_) {
            bool isCharged = (holdDuration_ >= gGameSettings.chargeTime);
            pointedObject_->shotAt = true;
            pointedObject_->chargedShot = isCharged;

            // Spawn floating message where teapot was destroyed
            FloatingMessage msg;
            msg.position = pointedObject_->position;
            msg.text = "You Found Russel's Teapot!";
            msg.timeRemaining = 2.0f;
            floatingMessages_.push_back(msg);

#if defined(__ANDROID__)
            triggerAndroidBanner("You Found Russel's Teapot!");
#endif
        }

        isHeld_ = false;
        holdDuration_ = 0.0f;
        pointedObject_ = nullptr;
    }
}

void TapInputHandler::update(float dt, int screenWidth, int screenHeight, const float viewMatrix[16], const float projectionMatrix[16], std::vector<std::unique_ptr<RigidBody>> &objects) {
    // Update Hitmarker timer
    if (hitmarker_.timeRemaining > 0.0f) {
        hitmarker_.timeRemaining -= dt;
    }

    // Update Floating Messages timer
    for (auto it = floatingMessages_.begin(); it != floatingMessages_.end();) {
        it->timeRemaining -= dt;
        if (it->timeRemaining <= 0.0f) {
            it = floatingMessages_.erase(it);
        } else {
            ++it;
        }
    }

    // Process destroyed objects (teapots where shotAt == true)
    for (auto it = objects.begin(); it != objects.end();) {
        if ((*it)->shotAt) {
            if (pointedObject_ == it->get()) {
                pointedObject_ = nullptr;
            }
            it = objects.erase(it);
        } else {
            ++it;
        }
    }

    if (!isHeld_ || gGameSettings.gameState != GameState::PLAYING) {
        pointedObject_ = nullptr;
        return;
    }

    holdDuration_ += dt;

    // Reset pointedAt flag on all teapots
    for (auto &object : objects) {
        object->pointedAt = false;
    }

    // Raycast into 3D world from touch point (tapX_, tapY_)
    float ndcX = (2.0f * tapX_ / (float)screenWidth) - 1.0f;
    float ndcY = 1.0f - (2.0f * tapY_ / (float)screenHeight);

    float aspect = (screenHeight > 0) ? (float)screenWidth / (float)screenHeight : 1.0f;
    float fovRad = 65.0f * 0.5f * (3.1415926535f / 180.0f);
    float f = 1.0f / std::tan(fovRad);

    // Ray direction in camera space
    Vec3 rayCam(ndcX * aspect / f, ndcY / f, -1.0f);
    rayCam = rayCam.normalized();

    // Ray direction in world space using inverse/transpose of viewRotation
    Vec3 rayWorld(
        viewMatrix[0] * rayCam.x + viewMatrix[1] * rayCam.y + viewMatrix[2] * rayCam.z,
        viewMatrix[4] * rayCam.x + viewMatrix[5] * rayCam.y + viewMatrix[6] * rayCam.z,
        viewMatrix[8] * rayCam.x + viewMatrix[9] * rayCam.y + viewMatrix[10] * rayCam.z
    );
    rayWorld = rayWorld.normalized();

    // Ray origin is camera position (derived from viewMatrix translation)
    Vec3 rayOrigin(
        -(viewMatrix[0] * viewMatrix[12] + viewMatrix[1] * viewMatrix[13] + viewMatrix[2] * viewMatrix[14]),
        -(viewMatrix[4] * viewMatrix[12] + viewMatrix[5] * viewMatrix[13] + viewMatrix[6] * viewMatrix[14]),
        -(viewMatrix[8] * viewMatrix[12] + viewMatrix[9] * viewMatrix[13] + viewMatrix[10] * viewMatrix[14])
    );

    RigidBody* closestObject = nullptr;
    float closestDist = 1e9f;

    for (auto &object : objects) {
        Vec3 toObject = object->position - rayOrigin;
        float t = Vec3::dot(toObject, rayWorld);
        if (t > 0.0f) {
            Vec3 closestPoint = rayOrigin + rayWorld * t;
            float distSq = (object->position - closestPoint).lengthSq();
            float hitRadius = object->radius * object->scale * 0.5f; // this should be made into a switch case later

            if (distSq <= hitRadius * hitRadius && t < closestDist) {
                closestDist = t;
                closestObject = object.get();
            }
        }
    }

    if (closestObject) {
        pointedObject_ = closestObject;
        pointedObject_->pointedAt = true;
    } else {
        pointedObject_ = nullptr;
    }
}
