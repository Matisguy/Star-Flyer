#include "GameSettings.h"

GameSettings gGameSettings;

void applyGameState(GameState state) {
    gGameSettings.gameState = state;
    if (state == GameState::PAUSED) {
        gGameSettings.isPaused = true;
    } else if (state == GameState::PLAYING) {
        gGameSettings.isPaused = false;
    }
}
