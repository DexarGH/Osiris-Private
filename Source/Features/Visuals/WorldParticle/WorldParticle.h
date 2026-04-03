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
        ensureParticleCount();
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
        }
    }

    void ensureParticleCount() const
    {
        auto& state = getState();
        const auto targetCount = static_cast<std::uint16_t>(GET_CONFIG_VAR(world_particle_vars::Count));

        auto&& containerPanel = hookContext.template make<PanoramaUiEngine>().getPanelFromHandle(state.containerPanelHandle);
        auto* rawContainerPanel = static_cast<cs2::CUIPanel*>(containerPanel);

        while (state.activeCount < targetCount) {
            auto&& particlePanel = hookContext.panelFactory().createPanel(rawContainerPanel).uiPanel();
            particlePanel.setWidth(cs2::CUILength::pixels(1.0f));
            particlePanel.setHeight(cs2::CUILength::pixels(1.0f));
            state.particles[state.activeCount].panelHandle = particlePanel.getHandle();
            state.particles[state.activeCount].active = false;
            resetParticle(state.particles[state.activeCount], state.activeCount);
            state.particles[state.activeCount].active = true;
            state.activeCount++;
        }
    }

    void resetParticle(ParticleData& particle, std::uint16_t idx) const
    {
        const auto mode = GET_CONFIG_VAR(world_particle_vars::ModeType);
        
        if (mode == world_particle_vars::Mode::Rain) {
            particle.screenX = -1.0f + static_cast<float>(rand() % 2000) / 1000.0f;
            particle.screenY = -1.2f - static_cast<float>(rand() % 300) / 1000.0f;
            particle.speed = 0.003f + static_cast<float>(rand() % 500) / 100000.0f;
        } else {
            particle.screenX = 0.0f;
            particle.screenY = 0.0f;
            particle.speed = 0.001f + static_cast<float>(idx % 100) / 50000.0f;
        }
        
        particle.opacity = 1.0f;
    }

    void updateParticles() const
    {
        auto& state = getState();
        const auto targetCount = static_cast<std::uint16_t>(GET_CONFIG_VAR(world_particle_vars::Count));
        const auto mode = GET_CONFIG_VAR(world_particle_vars::ModeType);
        const auto colorR = static_cast<std::uint8_t>(GET_CONFIG_VAR(world_particle_vars::ColorR));
        const auto colorG = static_cast<std::uint8_t>(GET_CONFIG_VAR(world_particle_vars::ColorG));
        const auto colorB = static_cast<std::uint8_t>(GET_CONFIG_VAR(world_particle_vars::ColorB));
        const auto particleType = GET_CONFIG_VAR(world_particle_vars::Type);

        const float size = getParticleSize(particleType);
        const float borderRadius = getBorderRadius(particleType, size);

        cs2::Vector playerHeadPos{};
        bool hasPlayerPos = false;
        if (mode == world_particle_vars::Mode::Tail) {
            auto&& localPlayer = hookContext.activeLocalPlayerPawn();
            if (localPlayer) {
                const auto origin = localPlayer.absOrigin();
                if (origin.hasValue()) {
                    playerHeadPos = origin.value();
                    playerHeadPos.z += 64.0f;
                    hasPlayerPos = true;
                }
            }
        }

        for (std::uint16_t i = 0; i < targetCount && i < state.activeCount; ++i) {
            auto& particle = state.particles[i];
            if (!particle.panelHandle.isValid())
                continue;

            auto&& panel = hookContext.template make<PanoramaUiEngine>().getPanelFromHandle(particle.panelHandle);
            
            panel.setWidth(cs2::CUILength::pixels(size));
            panel.setHeight(cs2::CUILength::pixels(size));
            panel.setBorderRadius(cs2::CUILength::pixels(borderRadius));
            panel.setBackgroundColor(cs2::Color{colorR, colorG, colorB, static_cast<std::uint8_t>(particle.opacity * 255.0f)});

            if (mode == world_particle_vars::Mode::Rain) {
                particle.screenY += particle.speed;
                
                if (particle.screenY > 1.2f) {
                    resetParticle(particle, i);
                }
            } else {
                if (hasPlayerPos) {
                    auto&& clip = hookContext.template make<WorldToClipSpaceConverter>().toClipSpace(playerHeadPos);
                    if (clip.onScreen()) {
                        const auto ndc = clip.toNormalizedDeviceCoordinates();
                        
                        const float offset = static_cast<float>(i) * 0.002f;
                        const float angle = static_cast<float>(i) * 0.15f;
                        
                        // NDC X/Y already in percent form from getX()/getY()
                        const float baseX = (ndc.getX().m_flValue / 100.0f) * 2.0f - 1.0f;
                        const float baseY = 1.0f - (ndc.getY().m_flValue / 100.0f) * 2.0f;
                        
                        particle.screenX += (baseX - particle.screenX) * 0.05f + std::cos(angle) * offset;
                        particle.screenY += (baseY - particle.screenY) * 0.05f + std::sin(angle) * offset * 0.5f;
                        
                        const auto dist = std::sqrt((particle.screenX - baseX) * (particle.screenX - baseX) + 
                                                   (particle.screenY - baseY) * (particle.screenY - baseY));
                        particle.opacity = std::max(0.3f, 1.0f - dist / 0.3f);
                    }
                }
            }

            panel.setZIndex(-1);
            PanoramaTransformations{
                hookContext.panoramaTransformFactory().translate(
                    cs2::CUILength::percent((particle.screenX + 1.0f) * 50.0f),
                    cs2::CUILength::percent((-particle.screenY + 1.0f) * 50.0f)
                )
            }.applyTo(panel);
            
            panel.setVisible(true);
        }

        for (std::uint16_t i = targetCount; i < state.activeCount && i < 1000; ++i) {
            if (state.particles[i].panelHandle.isValid()) {
                hookContext.template make<PanoramaUiEngine>().getPanelFromHandle(state.particles[i].panelHandle).setVisible(false);
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
