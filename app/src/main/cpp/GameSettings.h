#ifndef STARFLYER_GAME_SETTINGS_H
#define STARFLYER_GAME_SETTINGS_H

enum class GameState {
    MAIN_MENU = 0,
    LEVEL_SELECT = 1,
    PLAYING = 2,
    PAUSED = 3,
    WORKSHOP = 4
};

struct GameSettings {
    GameState gameState = GameState::MAIN_MENU;
    float musicVolume = 0.8f;
    float sfxVolume = 1.0f;
    float turnSensitivity = 3.0f;
    float rollSensitivity = 1.0f;
    bool isPaused = false;
    bool calibrationRequested = false;
};

extern GameSettings gGameSettings;

#endif // STARFLYER_GAME_SETTINGS_H
