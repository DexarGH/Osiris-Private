#pragma once

#include <cmath>
#include <GameClient/Panorama/PanelHandle.h>
#include <GameClient/Panorama/PanoramaUiEngine.h>
#include <GameClient/Panorama/PanoramaUiPanel.h>
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
            hookContext.template make<PanoramaUiEngine>().deletePanelByHandle(particle.panelHandle);
        }
        hookContext.template make<PanoramaUiEngine>().deletePanelByHandle(state.containerPanelHandle);
    }

private:
    void hideAll() const
    {
        auto& state = getState();
        for (auto& particle : state.particles) {
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
            initParticles();
        }
    }

    void initParticles() const
    {
        auto& state = getState();
        const auto targetCount = static_cast<std::uint16_t>(GET_CONFIG_VAR(world_particle_vars::Count));

        while (state.activeCount < targetCount) {
            const auto idx = state.activeCount;
            auto& particle = state.particles[idx];
            
            if (!particle.panelHandle.isValid()) {
                auto&& particlePanel = hookContext.panelFactory().createPanel(hookContext.hud().getHudReticle()).uiPanel();
                particlePanel.setWidth(cs2::CUILength::pixels(8.0f));
                particlePanel.setHeight(cs2::CUILength::pixels(8.0f));
                particlePanel.setBorderRadius(cs2::CUILength::pixels(4.0f));
                particle.panelHandle = particlePanel.getHandle();
            }

            resetParticle(particle, idx);
            state.activeCount++;
        }
    }

    void resetParticle(ParticleData& particle, std::uint16_t idx) const
    {
        const auto mode = GET_CONFIG_VAR(world_particle_vars::ModeType);
        
        if (mode == world_particle_vars::Mode::Rain) {
            particle.x = static_cast<float>(rand() % 1920);
            particle.y = -20.0f - static_cast<float>(rand() % 500);
            particle.speed = 3.0f + static_cast<float>(rand() % 500) / 100.0f;
        } else {
            particle.x = 960.0f;
            particle.y = 540.0f;
            particle.speed = 1.0f + static_cast<float>(idx % 100) / 50.0f;
        }
        
        particle.opacity = 1.0f;
        particle.active = true;
    }

    void updateParticles() const
    {
        auto& state = getState();
        const auto targetCount = static_cast<std::uint16_t>(GET_CONFIG_VAR(world_particle_vars::Count));
        const auto mode = GET_CONFIG_VAR(world_particle_vars::ModeType);
        const auto colorR = GET_CONFIG_VAR(world_particle_vars::ColorR);
        const auto colorG = GET_CONFIG_VAR(world_particle_vars::ColorG);
        const auto colorB = GET_CONFIG_VAR(world_particle_vars::ColorB);
        const auto particleType = GET_CONFIG_VAR(world_particle_vars::Type);

        const float size = getParticleSize(particleType);
        const float borderRadius = getBorderRadius(particleType, size);

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
                particle.y += particle.speed;
                
                if (particle.y > 1100.0f) {
                    resetParticle(particle, i);
                }
            } else {
                // Tail mode - simple circular motion
                const float angle = static_cast<float>(i) * 0.1f + particle.speed;
                const float radius = 50.0f + static_cast<float>(i) * 2.0f;
                particle.x = 960.0f + std::cos(angle) * radius;
                particle.y = 540.0f + std::sin(angle) * radius;
            }

            panel.setPosition(cs2::CUILength::pixels(particle.x), cs2::CUILength::pixels(particle.y));
            panel.setVisible(true);
        }

        for (std::uint16_t i = targetCount; i < state.activeCount && i < 1000; ++i) {
            hookContext.template make<PanoramaUiEngine>().getPanelFromHandle(state.particles[i].panelHandle).setVisible(false);
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
