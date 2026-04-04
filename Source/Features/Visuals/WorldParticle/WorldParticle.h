#pragma once

#include <cstdio>
#include <cstring>

#ifdef __linux__
#include <dlfcn.h>
#endif

#include <CS2/Panorama/CUILength.h>
#include <CS2/Panorama/CUIPanel.h>
#include <CS2/Panorama/StyleEnums.h>
#include <GameClient/Panorama/PanelAlignmentParams.h>
#include <GameClient/Panorama/PanelHandle.h>
#include <GameClient/Panorama/PanelShadowParams.h>
#include <GameClient/Panorama/PanoramaTransformations.h>
#include <GameClient/Panorama/PanoramaUiEngine.h>
#include <GameClient/Panorama/PanoramaUiPanel.h>
#include <GameClient/WorldToScreen/ViewToProjectionMatrix.h>
#include <GameClient/WorldToScreen/WorldToClipSpaceConverter.h>
#include <Utils/ColorUtils.h>
#include <Utils/Lvalue.h>
#include "WorldParticleConfigVariables.h"
#include "WorldParticleState.h"

#include <cstdio>
#include <cstring>

static const char* getParticleSvgPath(world_particle_vars::ParticleType type) noexcept
{
    // Заготовка для будущих типов — сейчас только Star
    switch (type) {
        case world_particle_vars::ParticleType::Star: return "s2r://panorama/images/icons/ui/star.svg";
        default: return "s2r://panorama/images/icons/ui/star.svg";
    }
}

struct WorldParticlePanel {
    [[nodiscard]] static float getScale(float z, float fovScale) noexcept
    {
        return std::clamp(500.0f / (z / fovScale + 400.0f), 0.3f, 1.0f);
    }

    [[nodiscard]] static float getOpacity(float timeAlive, float maxLife) noexcept
    {
        const float ratio = timeAlive / maxLife;
        if (ratio >= 1.0f)
            return 0.0f;
        return 1.0f - ratio;
    }
};

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

        auto&& localPlayer = hookContext.activeLocalPlayerPawn();
        if (!localPlayer) {
            hideAll();
            return;
        }

        const auto origin = localPlayer.absOrigin();
        if (!origin.hasValue()) {
            hideAll();
            return;
        }

        // Если тип изменился — пересоздаём все панели (заготовка для будущих типов)
        auto& state = getState();
        if (state.lastParticleType != GET_CONFIG_VAR(world_particle_vars::Type)) {
            state.lastParticleType = GET_CONFIG_VAR(world_particle_vars::Type);
            for (auto& particle : state.particles) {
                if (particle.panelHandle.isValid()) {
                    hookContext.template make<PanoramaUiEngine>().deletePanelByHandle(particle.panelHandle);
                    particle.panelHandle = {};
                    particle.active = false;
                }
            }
            state.particleCount = 0;
        }

        // Проверяем движение
        if (!state.lastPosInitialized) {
            state.lastPlayerX = origin.value().x;
            state.lastPlayerY = origin.value().y;
            state.lastPlayerZ = origin.value().z;
            state.lastPosInitialized = true;
        }

        const float deltaX = origin.value().x - state.lastPlayerX;
        const float deltaY = origin.value().y - state.lastPlayerY;
        const float deltaZ = origin.value().z - state.lastPlayerZ;
        const float movementSpeed = std::sqrt(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ);

        state.lastPlayerX = origin.value().x;
        state.lastPlayerY = origin.value().y;
        state.lastPlayerZ = origin.value().z;

        if (movementSpeed < 0.05f) {
            // Не скрываем мгновенно — позволяем частицам дожить и исчезнуть плавно
            updateActiveParticles();
            return;
        }

        // Спавним частицу в позиции игрока
        spawnParticle(origin.value());

        // Обновляем активные
        updateActiveParticles();
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

    void spawnParticle(const cs2::Vector& origin) const
    {
        auto& state = getState();
        const auto targetCount = static_cast<std::uint16_t>(GET_CONFIG_VAR(world_particle_vars::Count));

        // Находим свободный слот
        for (std::uint16_t i = 0; i < state.particleCount; ++i) {
            auto& particle = state.particles[i];
            if (!particle.active || particle.opacity <= 0.0f) {
                particle.origin = origin;
                particle.spawnTime = hookContext.globalVars().curtime().valueOr(0.0f);
                particle.maxLife = 2.0f;
                particle.active = true;
                if (!particle.panelHandle.isValid())
                    createParticlePanel(particle);
                return;
            }
        }

        // Создаём новый
        if (state.particleCount < targetCount) {
            auto& particle = state.particles[state.particleCount];
            particle.origin = origin;
            particle.spawnTime = hookContext.globalVars().curtime().valueOr(0.0f);
            particle.maxLife = 2.0f;
            particle.active = true;
            createParticlePanel(particle);
            state.particleCount++;
        }
    }

    void createParticlePanel(ParticleData& particle) const
    {
        auto& state = getState();
        if (!state.containerValid) {
            auto&& container = hookContext.panelFactory().createPanel(hookContext.hud().getHudReticle()).uiPanel();
            if (container) {
                container.fitParent();
                state.containerPanelHandle = container.getHandle();
                state.containerRaw = static_cast<cs2::CUIPanel*>(container);
                state.containerValid = true;
            }
        }
        
        if (!state.containerRaw)
            return;
        
        auto&& panel = hookContext.panelFactory().createPanel(state.containerRaw).uiPanel();
        panel.setWidth(cs2::CUILength::pixels(64.0f));
        panel.setHeight(cs2::CUILength::pixels(64.0f));
        panel.setPosition(cs2::CUILength::pixels(-32.0f), cs2::CUILength::pixels(-64.0f));
        panel.setTransformOrigin(cs2::CUILength::percent(50), cs2::CUILength::percent(100));
        panel.setVisible(false);

        auto&& imagePanel = hookContext.panelFactory().createImagePanel(panel);
        const auto particleType = GET_CONFIG_VAR(world_particle_vars::Type);
        imagePanel.setImageSvg(getParticleSvgPath(particleType), 64);

        auto&& uiPanel = imagePanel.uiPanel();
        uiPanel.setAlign(PanelAlignmentParams{cs2::k_EHorizontalAlignmentCenter, cs2::k_EVerticalAlignmentBottom});
        
        const auto particleColor = color::HSBtoRGB(
            color::HueInteger{static_cast<std::uint16_t>(GET_CONFIG_VAR(world_particle_vars::ColorHue))},
            color::Saturation{0.7f},
            color::Brightness{0.9f}
        );
        
        uiPanel.setWashColor(particleColor);
        uiPanel.setImageShadow(PanelShadowParams{
            .horizontalOffset{cs2::CUILength::pixels(0)},
            .verticalOffset{cs2::CUILength::pixels(0)},
            .blurRadius{cs2::CUILength::pixels(1)},
            .strength = 3,
            .color{cs2::kColorBlack}
        });

        particle.panelHandle = panel.getHandle();
    }

    void updateParticleColors() const
    {
        auto& state = getState();
        const auto particleColor = color::HSBtoRGB(
            color::HueInteger{static_cast<std::uint16_t>(GET_CONFIG_VAR(world_particle_vars::ColorHue))},
            color::Saturation{0.7f},
            color::Brightness{0.9f}
        );

        for (std::uint16_t i = 0; i < state.particleCount; ++i) {
            auto& particle = state.particles[i];
            if (particle.panelHandle.isValid() && particle.active) {
                auto&& panel = hookContext.template make<PanoramaUiEngine>().getPanelFromHandle(particle.panelHandle);
                panel.setWashColor(particleColor);
            }
        }
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
        if (!state.containerValid) {
            auto&& container = hookContext.panelFactory().createPanel(hookContext.hud().getHudReticle()).uiPanel();
            if (container) {
                container.fitParent();
                state.containerPanelHandle = container.getHandle();
                state.containerRaw = static_cast<cs2::CUIPanel*>(container);
                state.containerValid = true;
            }
        }
    }

    void updateActiveParticles() const
    {
        auto& state = getState();
        const auto curtime = hookContext.globalVars().curtime();
        if (!curtime.hasValue())
            return;

        for (std::uint16_t i = 0; i < state.particleCount; ++i) {
            auto& particle = state.particles[i];
            if (!particle.active || !particle.panelHandle.isValid())
                continue;

            const float timeAlive = curtime.value() - particle.spawnTime;
            const float opacity = WorldParticlePanel::getOpacity(timeAlive, particle.maxLife);

            if (opacity <= 0.0f) {
                particle.active = false;
                hookContext.template make<PanoramaUiEngine>().getPanelFromHandle(particle.panelHandle).setVisible(false);
                continue;
            }

            const auto clipSpace = hookContext.template make<WorldToClipSpaceConverter>().toClipSpace(particle.origin);
            if (!clipSpace.onScreen()) {
                hookContext.template make<PanoramaUiEngine>().getPanelFromHandle(particle.panelHandle).setVisible(false);
                continue;
            }

            auto&& panel = hookContext.template make<PanoramaUiEngine>().getPanelFromHandle(particle.panelHandle);

            const float fovScale = ViewToProjectionMatrix{hookContext}.getFovScale();
            const float scale = WorldParticlePanel::getScale(clipSpace.z, fovScale);

            const auto deviceCoordinates = clipSpace.toNormalizedDeviceCoordinates();

            PanoramaTransformations{
                hookContext.panoramaTransformFactory().scale(scale),
                hookContext.panoramaTransformFactory().translate(deviceCoordinates.getX(), deviceCoordinates.getY())
            }.applyTo(panel);

            panel.setOpacity(opacity);
            panel.setZIndex(-clipSpace.z);
            panel.setVisible(true);
        }
    }

    [[nodiscard]] auto& getState() const
    {
        return hookContext.featuresStates().visualFeaturesStates.worldParticleState;
    }

    HookContext& hookContext;
};
