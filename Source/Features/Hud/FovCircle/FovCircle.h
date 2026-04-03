#pragma once

#include <Features/Aimbot/AimbotConfigVariables.h>
#include <GameClient/Panorama/PanelHandle.h>
#include <GameClient/Panorama/PanoramaUiEngine.h>
#include <GameClient/Panorama/PanoramaUiPanel.h>
#include <Utils/Lvalue.h>
#include "FovCircleConfigVariables.h"

template <typename HookContext>
class FovCircle {
public:
    explicit FovCircle(HookContext& hookContext) noexcept
        : hookContext{hookContext}
    {
    }

    void update() const
    {
        if (!GET_CONFIG_VAR(fov_circle_vars::Enabled)) {
            hidePanel();
            return;
        }

        const auto fovValue = static_cast<float>(static_cast<std::uint16_t>(GET_CONFIG_VAR(aimbot::Fov)));
        const auto thickness = static_cast<float>(static_cast<std::uint16_t>(GET_CONFIG_VAR(fov_circle_vars::Thickness)));
        const auto colorType = GET_CONFIG_VAR(fov_circle_vars::Color);

        // Конвертируем угловой FOV в радиус в пикселях
        // aimbot использует: fovLimit = tan(FOV/2) в NDC координатах
        // NDC range [-1, 1] = screen_height пикселей
        // radius_pixels = tan(FOV/2) * (screen_height / 2)
        // Примечание: используем коррекцию 1.15 для компенсации масштабирования Panorama UI
        const auto fovRad = fovValue * (3.14159265f / 180.0f);
        const auto fovLimit = std::tan(fovRad * 0.5f);

        // Используем стандартную высоту экрана (1080p)
        const float screenHeight = 1080.0f;
        const float screenWidth = 1920.0f;

        // Используем минимальную ось для круглой формы
        const auto minDimension = std::min(screenWidth, screenHeight);
        const auto radius = fovLimit * (minDimension * 0.5f) * 1.15f;

        auto&& panel = getPanel();
        panel.setVisible(true);
        panel.setWidth(cs2::CUILength::pixels(radius * 2.0f));
        panel.setHeight(cs2::CUILength::pixels(radius * 2.0f));
        panel.setBorderRadius(cs2::CUILength::pixels(radius));
        panel.setBorder(cs2::CUILength::pixels(thickness), getColor(colorType));
        panel.setBackgroundColor(cs2::Color{0, 0, 0, 0});
        panel.setAlign(PanelAlignmentParams{
            .horizontalAlignment = cs2::k_EHorizontalAlignmentCenter,
            .verticalAlignment = cs2::k_EVerticalAlignmentCenter});
    }

    void onUnload() const
    {
        hookContext.template make<PanoramaUiEngine>().deletePanelByHandle(state().panelHandle);
    }

private:
    void hidePanel() const
    {
        hookContext.template make<PanelHandle>(state().panelHandle).get().hide();
    }

    [[nodiscard]] cs2::Color getColor(fov_circle_vars::ColorType colorType) const noexcept
    {
        switch (colorType) {
            case fov_circle_vars::ColorType::Green:   return cs2::Color{0, 255, 0, 255};
            case fov_circle_vars::ColorType::Red:     return cs2::Color{255, 0, 0, 255};
            case fov_circle_vars::ColorType::Blue:    return cs2::Color{0, 0, 255, 255};
            case fov_circle_vars::ColorType::Yellow:  return cs2::Color{255, 255, 0, 255};
            case fov_circle_vars::ColorType::Cyan:    return cs2::Color{0, 255, 255, 255};
            case fov_circle_vars::ColorType::Magenta: return cs2::Color{255, 0, 255, 255};
            case fov_circle_vars::ColorType::White:   return cs2::Color{255, 255, 255, 255};
            default: return cs2::Color{0, 255, 0, 255};
        }
    }

    [[nodiscard]] decltype(auto) getPanel() const
    {
        return hookContext.template make<PanelHandle>(state().panelHandle).getOrInit([this]() {
            auto&& panel = hookContext.panelFactory().createPanel(hookContext.hud().getHudReticle()).uiPanel();
            state().panelHandle = panel.getHandle();
            return panel;
        });
    }

    [[nodiscard]] auto& state() const
    {
        return hookContext.featuresStates().fovCircleState;
    }

    HookContext& hookContext;
};
