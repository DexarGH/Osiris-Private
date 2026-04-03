#pragma once

#include <cmath>

#include <CS2/Classes/Vector.h>
#include <GameClient/Panorama/PanelHandle.h>
#include <GameClient/Panorama/PanoramaTransformFactory.h>
#include <GameClient/Panorama/PanoramaTransformations.h>
#include <GameClient/Panorama/PanoramaUiEngine.h>
#include <GameClient/Panorama/PanoramaUiPanel.h>
#include <GameClient/WorldToScreen/WorldToClipSpaceConverter.h>
#include <Utils/Lvalue.h>
#include <Utils/Logger.h>
#include "WorldParticleConfigVariables.h"
#include "WorldParticleState.h"

template <typename HookContext>
class WorldParticle {
public:
    explicit WorldParticle(HookContext& hookContext) noexcept
        : hookContext{hookContext}
    {
        LOG("[WorldParticle] Constructor called");
    }

    void update() const
    {
        LOG("[WorldParticle] update() called, Enabled=%d", GET_CONFIG_VAR(world_particle_vars::Enabled));
        
        if (!GET_CONFIG_VAR(world_particle_vars::Enabled)) {
            hideAll();
            return;
        }

        ensureContainer();
        LOG("[WorldParticle] Container ensured");
        ensureParticles();
        LOG("[WorldParticle] Particles ensured, count=%d", getState().particleCount);
        updateParticles();
    }

    void onUnload() const
    {
        LOG("[WorldParticle] onUnload called");
        auto& state = getState();
        for (auto& particle : state.particles) {
            if (particle.panelHandle.isValid())
                hookContext.template make<PanoramaUiEngine>().deletePanelByHandle(particle.panelHandle);
        }
        hookContext.template make<PanoramaUiEngine>().deletePanelByHandle(state.containerPanelHandle);
    }

private:
    void hideAll() const
    {
        auto& state = getState();
        for (auto& particle : state.particles) {
            if (particle.panelHandle.isValid())
                hookContext.template make<PanoramaUiEngine>().getPanelFromHandle(particle.panelHandle).setVisible(false);
        }
    }

    void ensureContainer() const
    {
        auto& state = getState();
        if (!state.containerPanelHandle.isValid()) {
            // НЕ создаём контейнер - частицы будут прямыми детьми hudReticle
            state.rawContainerPanel = static_cast<cs2::CUIPanel*>(hookContext.hud().getHudReticle());
            LOG("[WorldParticle] Using hudReticle directly as parent: %p", state.rawContainerPanel);
        }
    }

    void ensureParticles() const
    {
        auto& state = getState();
        const auto targetCount = static_cast<std::uint16_t>(GET_CONFIG_VAR(world_particle_vars::Count));
        LOG("[WorldParticle] ensureParticles, current=%d, target=%d, rawParent=%p", state.particleCount, targetCount, state.rawContainerPanel);

        while (state.particleCount < targetCount) {
            if (state.particleCount >= 1000) break;
            
            auto& particle = state.particles[state.particleCount];
            LOG("[WorldParticle] Creating particle %d, rawContainer=%p", state.particleCount, state.rawContainerPanel);
            
            if (!particle.panelHandle.isValid() && state.rawContainerPanel) {
                auto&& particlePanel = hookContext.panelFactory().createPanel(state.rawContainerPanel).uiPanel();
                LOG("[WorldParticle] Particle panel created, valid=%d", particlePanel ? 1 : 0);
                
                // Устанавливаем панель в ЦЕНТР экрана изначально
                particlePanel.setPosition(cs2::CUILength::pixels(960.0f), cs2::CUILength::pixels(540.0f));
                
                particle.panelHandle = particlePanel.getHandle();
            }
            
            resetParticle(particle, state.particleCount);
            particle.active = true;
            state.particleCount++;
        }
    }

    void resetParticle(ParticleData& particle, std::uint16_t idx) const
    {
        const auto mode = GET_CONFIG_VAR(world_particle_vars::ModeType);
        
        if (mode == world_particle_vars::Mode::Rain) {
            particle.screenX = -1.0f + static_cast<float>(rand() % 2000) / 1000.0f;
            particle.screenY = -1.5f - static_cast<float>(rand() % 500) / 1000.0f;
            particle.speed = 0.005f + static_cast<float>(rand() % 500) / 100000.0f;
            particle.opacity = 1.0f;
        } else {
            particle.screenX = 0.0f;
            particle.screenY = 0.0f;
            particle.speed = 0.0f;
            particle.velocityX = 0.0f;
            particle.velocityY = 0.0f;
            particle.opacity = 1.0f;
            particle.life = 0.0f;
        }
    }

    void updateParticles() const
    {
        auto& state = getState();
        const auto mode = GET_CONFIG_VAR(world_particle_vars::ModeType);
        const auto colorR = static_cast<std::uint8_t>(GET_CONFIG_VAR(world_particle_vars::ColorR));
        const auto colorG = static_cast<std::uint8_t>(GET_CONFIG_VAR(world_particle_vars::ColorG));
        const auto colorB = static_cast<std::uint8_t>(GET_CONFIG_VAR(world_particle_vars::ColorB));
        const auto particleType = GET_CONFIG_VAR(world_particle_vars::Type);

        const float size = getParticleSize(particleType);
        static_cast<void>(particleType);
        LOG("[WorldParticle] updateParticles: mode=%d, size=%.1f, color=%d,%d,%d", mode, size, colorR, colorG, colorB);

        cs2::Vector playerHeadPos{};
        bool hasPlayerPos = false;
        auto&& localPlayer = hookContext.activeLocalPlayerPawn();
        if (localPlayer) {
            const auto origin = localPlayer.absOrigin();
            if (origin.hasValue()) {
                playerHeadPos = origin.value();
                playerHeadPos.z += 64.0f;
                hasPlayerPos = true;
                LOG("[WorldParticle] Player pos: %.1f, %.1f, %.1f", playerHeadPos.x, playerHeadPos.y, playerHeadPos.z);
            }
        }

        // Обновляем первые 10 частиц с логом
        for (std::uint16_t i = 0; i < state.particleCount && i < 10; ++i) {
            auto& particle = state.particles[i];
            if (!particle.panelHandle.isValid()) {
                LOG("[WorldParticle] Particle %d: INVALID HANDLE", i);
                continue;
            }

            if (mode == world_particle_vars::Mode::Rain) {
                particle.screenY += particle.speed;
                
                if (particle.screenY > 1.5f) {
                    resetParticle(particle, i);
                }
            } else if (hasPlayerPos) {
                auto&& clip = hookContext.template make<WorldToClipSpaceConverter>().toClipSpace(playerHeadPos);
                if (clip.onScreen()) {
                    const auto ndc = clip.toNormalizedDeviceCoordinates();
                    const float baseX = (ndc.getX().m_flValue / 100.0f) * 2.0f - 1.0f;
                    const float baseY = 1.0f - (ndc.getY().m_flValue / 100.0f) * 2.0f;
                    
                    state.frameCounter++;
                    if (state.frameCounter >= 3) {
                        particle.screenX = baseX;
                        particle.screenY = baseY;
                        particle.velocityX = (static_cast<float>(rand() % 200) - 100.0f) / 50000.0f;
                        particle.velocityY = (static_cast<float>(rand() % 200) - 100.0f) / 50000.0f;
                        particle.life = 0.0f;
                        particle.opacity = 1.0f;
                        state.frameCounter = 0;
                    }
                    
                    particle.screenX += particle.velocityX;
                    particle.screenY += particle.velocityY;
                    particle.life += 0.01f;
                    
                    const auto maxLife = 2.0f;
                    if (particle.life >= maxLife) {
                        particle.opacity = 0.0f;
                    } else {
                        particle.opacity = 1.0f - (particle.life / maxLife);
                    }
                }
            }

            // Применяем визуальные параметры
            auto&& panel = hookContext.template make<PanoramaUiEngine>().getPanelFromHandle(particle.panelHandle);
            
            // Используем БОЛЬШИЕ частицы для теста
            const float testSize = 20.0f;
            panel.setWidth(cs2::CUILength::pixels(testSize));
            panel.setHeight(cs2::CUILength::pixels(testSize));
            panel.setBorderRadius(cs2::CUILength::pixels(testSize * 0.5f));
            
            // Ярко-жёлтый цвет для видимости
            panel.setBackgroundColor(cs2::Color{255, 255, 0, 255});
            panel.setZIndex(10000);
            
            // Просто setPosition каждый кадр - без PanoramaTransformations
            const float pixelX = 960.0f + particle.screenX * 960.0f;
            const float pixelY = 540.0f - particle.screenY * 540.0f;
            panel.setPosition(cs2::CUILength::pixels(pixelX), cs2::CUILength::pixels(pixelY));
            panel.setVisible(true);
            
            if (i < 3) {
                LOG("[WorldParticle] Particle %d: ndc=(%.3f, %.3f) -> pixel=(%.1f, %.1f)", 
                    i, particle.screenX, particle.screenY, pixelX, pixelY);
            }
        }
    }

    [[nodiscard]] float getParticleSize(world_particle_vars::ParticleType type) const noexcept
    {
        switch (type) {
            case world_particle_vars::ParticleType::Star: return 12.0f;
            case world_particle_vars::ParticleType::Snow: return 8.0f;
            case world_particle_vars::ParticleType::Bloom: return 16.0f;
            case world_particle_vars::ParticleType::Dollar: return 14.0f;
            default: return 8.0f;
        }
    }

    [[nodiscard]] float getBorderRadius(world_particle_vars::ParticleType type, float size) const noexcept
    {
        switch (type) {
            case world_particle_vars::ParticleType::Star: return size * 0.2f;
            case world_particle_vars::ParticleType::Snow: return size * 0.5f;
            case world_particle_vars::ParticleType::Bloom: return size * 0.1f;
            case world_particle_vars::ParticleType::Dollar: return size * 0.15f;
            default: return size * 0.5f;
        }
    }

    [[nodiscard]] auto& getState() const
    {
        return hookContext.featuresStates().visualFeaturesStates.worldParticleState;
    }

    HookContext& hookContext;
};
