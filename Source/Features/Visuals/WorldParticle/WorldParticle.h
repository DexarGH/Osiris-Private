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
#include "WorldParticleConfigVariables.h"
#include "WorldParticleState.h"

template <typename HookContext>
class WorldParticle {
public:
    explicit WorldParticle(HookContext& hookContext) noexcept
        : hookContext{hookContext}
    {
    }

    void update() const
    {
        if (!GET_CONFIG_VAR(world_particle_vars::Enabled)) {
            hideAll();
            return;
        }

        ensureContainer();
        updateParticles();
    }

    void onUnload() const
    {
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
            auto&& panel = hookContext.panelFactory().createPanel(hookContext.hud().getHudReticle()).uiPanel();
            panel.fitParent();
            state.containerPanelHandle = panel.getHandle();
            state.rawContainerPanel = static_cast<cs2::CUIPanel*>(panel);
            state.spawnIndex = 0;
            state.frameCounter = 0;
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
        const auto maxCount = static_cast<std::uint16_t>(GET_CONFIG_VAR(world_particle_vars::Count));

        const float size = getParticleSize(particleType);
        const float borderRadius = getBorderRadius(particleType, size);

        cs2::Vector playerHeadPos{};
        bool hasPlayerPos = false;
        auto&& localPlayer = hookContext.activeLocalPlayerPawn();
        if (localPlayer) {
            const auto origin = localPlayer.absOrigin();
            if (origin.hasValue()) {
                playerHeadPos = origin.value();
                playerHeadPos.z += 64.0f;
                hasPlayerPos = true;
            }
        }

        if (!state.rawContainerPanel)
            return;

        if (mode == world_particle_vars::Mode::Rain) {
            updateRainParticles(state, maxCount, size, borderRadius, colorR, colorG, colorB);
        } else if (hasPlayerPos) {
            updateTailParticles(state, maxCount, size, borderRadius, colorR, colorG, colorB, playerHeadPos);
        }
    }

    void updateRainParticles(auto& state, std::uint16_t maxCount, float size, float borderRadius,
                            std::uint8_t colorR, std::uint8_t colorG, std::uint8_t colorB) const
    {
        for (std::uint16_t i = 0; i < maxCount && i < 1000; ++i) {
            auto& particle = state.particles[i];
            
            if (!particle.panelHandle.isValid()) {
                auto&& particlePanel = hookContext.panelFactory().createPanel(state.rawContainerPanel).uiPanel();
                particle.panelHandle = particlePanel.getHandle();
                
                particle.screenX = -1.0f + static_cast<float>(rand() % 2000) / 1000.0f;
                particle.screenY = -1.5f - static_cast<float>(rand() % 500) / 1000.0f;
                particle.speed = 0.005f + static_cast<float>(rand() % 500) / 100000.0f;
                particle.opacity = 0.8f + static_cast<float>(rand() % 200) / 1000.0f;
            }

            particle.screenY += particle.speed;

            if (particle.screenY > 1.5f) {
                particle.screenX = -1.0f + static_cast<float>(rand() % 2000) / 1000.0f;
                particle.screenY = -1.5f - static_cast<float>(rand() % 500) / 1000.0f;
                particle.opacity = 0.8f + static_cast<float>(rand() % 200) / 1000.0f;
            }

            updateParticleVisuals(particle, size, borderRadius, colorR, colorG, colorB);
        }
    }

    void updateTailParticles(auto& state, std::uint16_t maxCount, float size, float borderRadius,
                            std::uint8_t colorR, std::uint8_t colorG, std::uint8_t colorB,
                            const cs2::Vector& playerPos) const
    {
        auto&& clip = hookContext.template make<WorldToClipSpaceConverter>().toClipSpace(playerPos);
        if (!clip.onScreen()) {
            for (std::uint16_t i = 0; i < maxCount && i < 1000; ++i) {
                if (state.particles[i].panelHandle.isValid()) {
                    hookContext.template make<PanoramaUiEngine>().getPanelFromHandle(state.particles[i].panelHandle).setVisible(false);
                }
            }
            return;
        }

        const auto ndc = clip.toNormalizedDeviceCoordinates();
        const float playerScreenX = (ndc.getX().m_flValue / 100.0f) * 2.0f - 1.0f;
        const float playerScreenY = 1.0f - (ndc.getY().m_flValue / 100.0f) * 2.0f;

        state.frameCounter++;
        const auto spawnInterval = 3;
        
        if (state.frameCounter >= spawnInterval && state.spawnIndex < maxCount && state.spawnIndex < 1000) {
            auto& particle = state.particles[state.spawnIndex];
            
            if (!particle.panelHandle.isValid()) {
                auto&& particlePanel = hookContext.panelFactory().createPanel(state.rawContainerPanel).uiPanel();
                particle.panelHandle = particlePanel.getHandle();
            }

            particle.screenX = playerScreenX + (static_cast<float>(rand() % 200) - 100.0f) / 1000.0f;
            particle.screenY = playerScreenY + (static_cast<float>(rand() % 200) - 100.0f) / 1000.0f;
            particle.velocityX = (static_cast<float>(rand() % 200) - 100.0f) / 50000.0f;
            particle.velocityY = (static_cast<float>(rand() % 200) - 100.0f) / 50000.0f;
            particle.opacity = 1.0f;
            particle.life = 0.0f;
            particle.active = true;
            
            state.spawnIndex++;
            state.frameCounter = 0;
        }

        for (std::uint16_t i = 0; i < state.spawnIndex && i < 1000; ++i) {
            auto& particle = state.particles[i];
            if (!particle.panelHandle.isValid() || !particle.active)
                continue;

            particle.screenX += particle.velocityX;
            particle.screenY += particle.velocityY;
            particle.life += 0.01f;

            const auto maxLife = 2.0f;
            if (particle.life >= maxLife) {
                particle.opacity = 0.0f;
                particle.active = false;
            } else {
                particle.opacity = 1.0f - (particle.life / maxLife);
            }

            updateParticleVisuals(particle, size, borderRadius, colorR, colorG, colorB);
        }
    }

    void updateParticleVisuals(ParticleData& particle, float size, float borderRadius,
                               std::uint8_t colorR, std::uint8_t colorG, std::uint8_t colorB) const
    {
        if (!particle.panelHandle.isValid())
            return;

        auto&& panel = hookContext.template make<PanoramaUiEngine>().getPanelFromHandle(particle.panelHandle);
        
        panel.setWidth(cs2::CUILength::pixels(size));
        panel.setHeight(cs2::CUILength::pixels(size));
        panel.setBorderRadius(cs2::CUILength::pixels(borderRadius));
        panel.setBackgroundColor(cs2::Color{
            static_cast<std::uint8_t>(colorR),
            static_cast<std::uint8_t>(colorG),
            static_cast<std::uint8_t>(colorB),
            static_cast<std::uint8_t>(particle.opacity * 255.0f)});

        panel.setZIndex(-1);
        PanoramaTransformations{
            hookContext.panoramaTransformFactory().translate(
                cs2::CUILength::percent((particle.screenX + 1.0f) * 50.0f),
                cs2::CUILength::percent((-particle.screenY + 1.0f) * 50.0f)
            )
        }.applyTo(panel);
        
        panel.setVisible(particle.opacity > 0.01f);
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
