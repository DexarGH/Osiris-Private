#pragma once

#include <CS2/Classes/Vector.h>
#include <CS2/Panorama/PanelHandle.h>

struct ParticleData {
    cs2::PanelHandle panelHandle{};
    cs2::PanelHandle imagePanelHandle{};
    cs2::Vector origin{};       // Точка спавна
    cs2::Vector currentPos{};   // Текущая позиция (для Rain — падает)
    float spawnTime{0.0f};
    float maxLife{2.0f};
    float opacity{1.0f};
    bool active{false};
};
