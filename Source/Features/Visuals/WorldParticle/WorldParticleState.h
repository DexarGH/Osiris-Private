#pragma once

#include <CS2/Panorama/CUIPanel.h>
#include <CS2/Panorama/PanelHandle.h>
#include <array>

struct ParticleData {
    cs2::PanelHandle panelHandle{};
    float screenX{0.0f};
    float screenY{0.0f};
    float speed{0.0f};
    float velocityX{0.0f};
    float velocityY{0.0f};
    float opacity{1.0f};
    float life{0.0f};
    bool active{false};
};

struct WorldParticleState {
    std::array<ParticleData, 1000> particles{};
    cs2::PanelHandle containerPanelHandle{};
    cs2::CUIPanel* rawContainerPanel{nullptr};
    std::uint16_t particleCount{0};
    std::uint16_t frameCounter{0};
    bool testPanelCreated{false};
};
