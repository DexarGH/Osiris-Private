#pragma once

#include <SDL/SdlDll.h>

struct TriggerBotState {
    bool enabled{false};
    sdl3::SDL_GetKeyboardState* getKeyboardState{nullptr};
    float lastShotTime{0.0f};
};
