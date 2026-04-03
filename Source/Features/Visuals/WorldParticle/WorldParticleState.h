#pragma once

#include <CS2/Panorama/PanelHandle.h>
#include <array>

struct ParticleData {
    cs2::PanelHandle panelHandle{};
    float x{0.0f};
    float y{0.0f};
    float speed{0.0f};
    float opacity{1.0f};
    bool active{false};
};

struct WorldParticleState {
    std::array<ParticleData, 1000> particles{};
    cs2::PanelHandle containerPanelHandle{};
    std::uint16_t activeCount{0};
};
