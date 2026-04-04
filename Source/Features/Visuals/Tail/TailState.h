#pragma once

#include <CS2/Panorama/PanelHandle.h>
#include <array>
#include "../ParticleData.h"
#include "TailConfigVariables.h"

struct TailState {
    std::array<ParticleData, 1000> particles{};
    cs2::PanelHandle containerPanelHandle{};
    cs2::CUIPanel* containerRaw{nullptr};
    bool containerValid{false};
    std::uint16_t particleCount{0};

    float lastPlayerX{0.0f};
    float lastPlayerY{0.0f};
    float lastPlayerZ{0.0f};
    bool lastPosInitialized{false};

    tail_vars::ParticleType lastParticleType{tail_vars::ParticleType::Star};
};
