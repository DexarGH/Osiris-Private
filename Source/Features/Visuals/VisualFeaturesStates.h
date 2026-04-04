#pragma once

#include "ModelGlow/ModelGlowState.h"
#include "PlayerInfoInWorld/PlayerInfoInWorldState.h"
#include "Rain/RainState.h"
#include "Tail/TailState.h"

struct VisualFeaturesStates {
    PlayerInfoInWorldState playerInfoInWorldState;
    ModelGlowState modelGlowState;
    RainState rainState;
    TailState tailState;
};
