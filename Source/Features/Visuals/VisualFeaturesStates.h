#pragma once

#include "ModelGlow/ModelGlowState.h"
#include "PlayerInfoInWorld/PlayerInfoInWorldState.h"
#include "WorldParticle/WorldParticleState.h"

struct VisualFeaturesStates {
    PlayerInfoInWorldState playerInfoInWorldState;
    ModelGlowState modelGlowState;
    WorldParticleState worldParticleState;
};
