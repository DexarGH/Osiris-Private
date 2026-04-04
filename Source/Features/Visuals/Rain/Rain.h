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
#include "RainConfigVariables.h"
#include "RainState.h"

static const char* getRainParticleSvgPath(rain_vars::ParticleType type) noexcept
{
    switch (type) {
        case rain_vars::ParticleType::Star: return "s2r://panorama/images/icons/ui/star.svg";
        default: return "s2r://panorama/images/icons/ui/star.svg";
    }
}

struct RainParticlePanel {
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
class Rain {
public:
    explicit Rain(HookContext& hookContext) noexcept
        : hookContext{hookContext}
    {
    }

    void update() const
    {
        if (!GET_CONFIG_VAR(rain_vars::Enabled)) {
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
        if (state.lastParticleType != GET_CONFIG_VAR(rain_vars::Type)) {
            state.lastParticleType = GET_CONFIG_VAR(rain_vars::Type);
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

        // Обновляем падающие частицы
        updateFallingParticles(origin.value());
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

    void updateFallingParticles(const cs2::Vector& playerOrigin) const
    {
        auto& state = getState();
        const auto curtime = hookContext.globalVars().curtime();
        if (!curtime.hasValue())
            return;

        const float deltaTime = 0.016f; // ~60 FPS
        const float speed = static_cast<float>(GET_CONFIG_VAR(rain_vars::Speed));
        const float tiltAngle = static_cast<float>(GET_CONFIG_VAR(rain_vars::TiltAngle));
        const float tiltRad = tiltAngle * 3.14159265f / 180.0f;
        const float liveTime = static_cast<float>(GET_CONFIG_VAR(rain_vars::LiveTime));
        
        // Обновляем позиции и проверяем время жизни
        for (std::uint16_t i = 0; i < state.particleCount; ++i) {
            auto& particle = state.particles[i];
            if (!particle.active || !particle.panelHandle.isValid())
                continue;

            // Двигаем частицу вниз
            particle.currentPos.z -= speed * deltaTime;
            // Наклон по X
            particle.currentPos.x += speed * std::sin(tiltRad) * deltaTime;

            const float timeAlive = curtime.value() - particle.spawnTime;

            // Проверяем время жизни
            if (timeAlive >= liveTime) {
                // Время жизни вышло - деактивируем
                particle.active = false;
                hookContext.template make<PanoramaUiEngine>().getPanelFromHandle(particle.panelHandle).setVisible(false);
                continue;
            }

            // Конвертируем в экранные координаты
            const auto clipSpace = hookContext.template make<WorldToClipSpaceConverter>().toClipSpace(particle.currentPos);
            if (!clipSpace.onScreen()) {
                hookContext.template make<PanoramaUiEngine>().getPanelFromHandle(particle.panelHandle).setVisible(false);
                continue;
            }

            auto&& panel = hookContext.template make<PanoramaUiEngine>().getPanelFromHandle(particle.panelHandle);

            const float fovScale = ViewToProjectionMatrix{hookContext}.getFovScale();
            const float scale = RainParticlePanel::getScale(clipSpace.z, fovScale);
            
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

        // Спавним новые частицы вместо деактивированных
        spawnRainParticles(playerOrigin);
    }

    void spawnRainParticles(const cs2::Vector& playerOrigin) const
    {
        auto& state = getState();
        const auto targetCount = static_cast<std::uint16_t>(GET_CONFIG_VAR(rain_vars::Count));
        const float spawnRadius = static_cast<float>(GET_CONFIG_VAR(rain_vars::SpawnRadius));
        const float skyHeight = 500.0f; // Высота спавна над игроком
        const auto curtime = hookContext.globalVars().curtime();
        if (!curtime.hasValue())
            return;

        // Перезапускаем деактивированные частицы
        for (std::uint16_t i = 0; i < state.particleCount; ++i) {
            auto& particle = state.particles[i];
            if (!particle.active) {
                // Случайная позиция в радиусе вокруг игрока
                const float randomX = playerOrigin.x + (static_cast<float>(std::rand()) / RAND_MAX * 2.0f - 1.0f) * spawnRadius;
                const float randomY = playerOrigin.y + (static_cast<float>(std::rand()) / RAND_MAX * 2.0f - 1.0f) * spawnRadius;
                const float spawnZ = playerOrigin.z + skyHeight;

                particle.origin = cs2::Vector{randomX, randomY, spawnZ};
                particle.currentPos = particle.origin;
                particle.spawnTime = curtime.value();
                particle.active = true;
                if (!particle.panelHandle.isValid())
                    createParticlePanel(particle);
                return; // Спавним по одной за вызов
            }
        }

        // Создаём новые слоты пока не достигнем targetCount
        if (state.particleCount < targetCount) {
            auto& particle = state.particles[state.particleCount];
            const float randomX = playerOrigin.x + (static_cast<float>(std::rand()) / RAND_MAX * 2.0f - 1.0f) * spawnRadius;
            const float randomY = playerOrigin.y + (static_cast<float>(std::rand()) / RAND_MAX * 2.0f - 1.0f) * spawnRadius;
            const float spawnZ = playerOrigin.z + skyHeight;

            particle.origin = cs2::Vector{randomX, randomY, spawnZ};
            particle.currentPos = particle.origin;
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
        const auto particleType = GET_CONFIG_VAR(rain_vars::Type);
        imagePanel.setImageSvg(getRainParticleSvgPath(particleType), 64);

        auto&& uiPanel = imagePanel.uiPanel();
        uiPanel.setAlign(PanelAlignmentParams{cs2::k_EHorizontalAlignmentCenter, cs2::k_EVerticalAlignmentBottom});

        const auto particleColor = color::HSBtoRGB(
            color::HueInteger{static_cast<std::uint16_t>(GET_CONFIG_VAR(rain_vars::ColorHue))},
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

    [[nodiscard]] auto& getState() const
    {
        return hookContext.featuresStates().visualFeaturesStates.rainState;
    }

    HookContext& hookContext;
};
