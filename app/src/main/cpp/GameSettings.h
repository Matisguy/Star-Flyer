#ifndef STARFLYER_GAME_SETTINGS_H
#define STARFLYER_GAME_SETTINGS_H

struct GameSettings {
    float turnSensitivity = 3.f;
    float rollSensitivity = 0.05f;
    bool isPaused = false;
    bool calibrationRequested = false;
};

extern GameSettings gGameSettings;

#endif // STARFLYER_GAME_SETTINGS_H
