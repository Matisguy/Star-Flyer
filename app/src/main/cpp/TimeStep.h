#ifndef STARFLYER_TIMESTEP_H
#define STARFLYER_TIMESTEP_H

#include <chrono>

class TimeStep {
public:
    TimeStep() : lastTime_(std::chrono::high_resolution_clock::now()) {}

    /**
     * Calculates the elapsed time in seconds since the last call to tick()
     */
    float tick() {
        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = currentTime - lastTime_;
        lastTime_ = currentTime;

        float dt = elapsed.count();
        // Clamp maximum delta time to prevent large physics skips during pauses or frame drops
        if (dt > 0.1f) {
            dt = 0.1f;
        }
        return dt;
    }

private:
    std::chrono::high_resolution_clock::time_point lastTime_;
};

#endif // STARFLYER_TIMESTEP_H
