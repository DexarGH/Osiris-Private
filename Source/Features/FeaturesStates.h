#pragma once

#include "Aimbot/AimbotState.h"
#include "Combat/SniperRifles/NoScopeInaccuracyVis/NoScopeInaccuracyVisState.h"
#include "Hud/FovCircle/FovCircleState.h"
#include "Hud/HudFeaturesStates.h"
#include "Visuals/VisualFeaturesStates.h"

struct FeaturesStates {
    AimbotState aimbotState;
    HudFeaturesStates hudFeaturesStates;
    VisualFeaturesStates visualFeaturesStates;
    NoScopeInaccuracyVisState noScopeInaccuracyVisState;
    FovCircleState fovCircleState;
};
