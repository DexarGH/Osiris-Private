#pragma once

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

        const auto fovValue = static_cast<float>(GET_CONFIG_VAR(fov_circle_vars::Fov));
        const auto thickness = static_cast<float>(GET_CONFIG_VAR(fov_circle_vars::Thickness));
        const auto colorType = GET_CONFIG_VAR(fov_circle_vars::Color);

        auto&& panel = getPanel();
        panel.setVisible(true);
        panel.setWidth(cs2::CUILength::pixels(fovValue * 2.0f));
        panel.setHeight(cs2::CUILength::pixels(fovValue * 2.0f));
        panel.setBorderRadius(cs2::CUILength::pixels(fovValue));
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
