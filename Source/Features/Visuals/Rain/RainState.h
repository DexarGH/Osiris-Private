#pragma once

#include <CS2/Panorama/PanelHandle.h>
#include <array>
#include "../ParticleData.h"
#include "RainConfigVariables.h"

struct RainState {
    std::array<ParticleData, 1000> particles{};
    cs2::PanelHandle containerPanelHandle{};
    cs2::CUIPanel* containerRaw{nullptr};
    bool containerValid{false};
    std::uint16_t particleCount{0};

    float lastSpawnTime{0.0f};
    rain_vars::ParticleType lastParticleType{rain_vars::ParticleType::Star};
};
