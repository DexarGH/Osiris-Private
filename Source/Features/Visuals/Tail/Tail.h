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
#include "TailConfigVariables.h"
#include "TailState.h"

static const char* getParticleSvgPath(tail_vars::ParticleType type) noexcept
{
    switch (type) {
        case tail_vars::ParticleType::Star: return "s2r://panorama/images/icons/ui/star.svg";
        default: return "s2r://panorama/images/icons/ui/star.svg";
    }
}

struct TailParticlePanel {
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
class Tail {
public:
    explicit Tail(HookContext& hookContext) noexcept
        : hookContext{hookContext}
    {
    }

    void update() const
    {
        if (!GET_CONFIG_VAR(tail_vars::Enabled)) {
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

        auto& state = getState();

        // Проверяем смену типа частиц
        if (state.lastParticleType != GET_CONFIG_VAR(tail_vars::Type)) {
            state.lastParticleType = GET_CONFIG_VAR(tail_vars::Type);
            for (auto& particle : state.particles) {
                if (particle.imagePanelHandle.isValid()) {
                    hookContext.template make<PanoramaUiEngine>().deletePanelByHandle(particle.imagePanelHandle);
                    particle.imagePanelHandle = {};
                }
                if (particle.panelHandle.isValid()) {
                    hookContext.template make<PanoramaUiEngine>().deletePanelByHandle(particle.panelHandle);
                    particle.panelHandle = {};
                    particle.active = false;
                }
            }
            state.particleCount = 0;
        }

        // Инициализируем позицию
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

        // Обновляем время жизни и отображение
        updateTailParticles();

        // Спавним только при движении
        if (movementSpeed >= 0.05f) {
            spawnTailParticles(origin.value());
        }
    }

    void onUnload() const
    {
        auto& state = getState();
        for (auto& particle : state.particles) {
            if (particle.imagePanelHandle.isValid())
                hookContext.template make<PanoramaUiEngine>().deletePanelByHandle(particle.imagePanelHandle);
            if (particle.panelHandle.isValid())
                hookContext.template make<PanoramaUiEngine>().deletePanelByHandle(particle.panelHandle);
        }
        hookContext.template make<PanoramaUiEngine>().deletePanelByHandle(state.containerPanelHandle);
    }

    void recreateAllParticles() const
    {
        auto& state = getState();
        
        for (auto& particle : state.particles) {
            if (particle.imagePanelHandle.isValid()) {
                hookContext.template make<PanoramaUiEngine>().deletePanelByHandle(particle.imagePanelHandle);
                particle.imagePanelHandle = {};
            }
            if (particle.panelHandle.isValid()) {
                hookContext.template make<PanoramaUiEngine>().deletePanelByHandle(particle.panelHandle);
                particle.panelHandle = {};
            }
            particle.active = false;
        }
        state.particleCount = 0;
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

    void updateTailParticles() const
    {
        auto& state = getState();
        const auto curtime = hookContext.globalVars().curtime();
        if (!curtime.hasValue())
            return;

        const float liveTime = static_cast<float>(GET_CONFIG_VAR(tail_vars::LiveTime));

        // Обновляем время жизни и отображение
        for (std::uint16_t i = 0; i < state.particleCount; ++i) {
            auto& particle = state.particles[i];
            if (!particle.active || !particle.panelHandle.isValid())
                continue;

            const float timeAlive = curtime.value() - particle.spawnTime;

            // Проверяем время жизни
            if (timeAlive >= liveTime) {
                particle.active = false;
                hookContext.template make<PanoramaUiEngine>().getPanelFromHandle(particle.panelHandle).setVisible(false);
                continue;
            }

            // Конвертируем в экранные координаты
            const auto clipSpace = hookContext.template make<WorldToClipSpaceConverter>().toClipSpace(particle.origin);
            
            // Скрываем частицы за спиной (z <= 0) или слишком близко к камере
            constexpr float minClipZ = 10.0f; // Минимальное расстояние от камеры
            if (clipSpace.z < minClipZ || !clipSpace.onScreen()) {
                hookContext.template make<PanoramaUiEngine>().getPanelFromHandle(particle.panelHandle).setVisible(false);
                continue;
            }

            auto&& panel = hookContext.template make<PanoramaUiEngine>().getPanelFromHandle(particle.panelHandle);

            const float fovScale = ViewToProjectionMatrix{hookContext}.getFovScale();
            const float scale = TailParticlePanel::getScale(clipSpace.z, fovScale);
            
            // Прозрачность на основе времени жизни
            const float lifeRatio = timeAlive / liveTime;
            const float opacity = 1.0f - lifeRatio;

            if (opacity <= 0.0f) {
                particle.active = false;
                panel.setVisible(false);
                continue;
            }

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

    void spawnTailParticles(const cs2::Vector& playerOrigin) const
    {
        auto& state = getState();
        const auto curtime = hookContext.globalVars().curtime();
        if (!curtime.hasValue())
            return;

        // Проверяем задержку спавна
        const float spawnRateSec = static_cast<float>(GET_CONFIG_VAR(tail_vars::SpawnRate)) / 1000.0f;
        if (curtime.value() - state.lastSpawnTime < spawnRateSec)
            return;

        state.lastSpawnTime = curtime.value();

        // Ищем деактивированную частицу
        for (std::uint16_t i = 0; i < state.particleCount; ++i) {
            auto& particle = state.particles[i];
            if (!particle.active) {
                particle.origin = playerOrigin;
                particle.spawnTime = curtime.value();
                particle.active = true;
                if (!particle.panelHandle.isValid())
                    createParticlePanel(particle);
                return;
            }
        }

        // Если нет свободных слотов — создаём новый
        if (state.particleCount < 1000) {
            auto& particle = state.particles[state.particleCount];
            particle.origin = playerOrigin;
            particle.spawnTime = curtime.value();
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
        const auto particleType = GET_CONFIG_VAR(tail_vars::Type);
        imagePanel.setImageSvg(getParticleSvgPath(particleType), 64);

        auto&& uiPanel = imagePanel.uiPanel();
        uiPanel.setAlign(PanelAlignmentParams{cs2::k_EHorizontalAlignmentCenter, cs2::k_EVerticalAlignmentBottom});

        const auto particleColor = color::HSBtoRGB(
            color::HueInteger{static_cast<std::uint16_t>(GET_CONFIG_VAR(tail_vars::ColorHue))},
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
        particle.imagePanelHandle = uiPanel.getHandle();
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
            const float opacity = TailParticlePanel::getOpacity(timeAlive, particle.maxLife);

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
            const float scale = TailParticlePanel::getScale(clipSpace.z, fovScale);

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
        return hookContext.featuresStates().visualFeaturesStates.tailState;
    }

    HookContext& hookContext;
};
