#pragma once

#include <CS2/Classes/Vector.h>
#include <CS2/Panorama/PanelHandle.h>
#include <array>
#include "WorldParticleConfigVariables.h"

struct ParticleData {
    cs2::PanelHandle panelHandle{};
    cs2::Vector origin{};
    float spawnTime{0.0f};
    float maxLife{2.0f};
    float opacity{1.0f};
    bool active{false};
};

struct WorldParticleState {
    std::array<ParticleData, 1000> particles{};
    cs2::PanelHandle containerPanelHandle{};
    cs2::CUIPanel* containerRaw{nullptr};
    bool containerValid{false};
    std::uint16_t particleCount{0};

    float lastPlayerX{0.0f};
    float lastPlayerY{0.0f};
    float lastPlayerZ{0.0f};
    bool lastPosInitialized{false};
    
    // Заготовка для отслеживания смены типа частиц
    world_particle_vars::ParticleType lastParticleType{world_particle_vars::ParticleType::Star};
};
