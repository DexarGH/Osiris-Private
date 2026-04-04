#pragma once

#include <concepts>
#include <cstdint>

#include <GameClient/Panorama/PanoramaUiPanel.h>
#include <GameClient/Panorama/Slider.h>
#include <Utils/StringBuilder.h>

template <typename HookContext>
class FloatSlider {
public:
    FloatSlider(HookContext& hookContext, cs2::CUIPanel* _panel) noexcept
        : hookContext{hookContext}
        , _panel{_panel}
    {
    }

    void updateSlider(float value) const noexcept
    {
        panel().children()[0].clientPanel().template as<Slider>().setValue(value);
    }

    void updateTextEntry(float value) const noexcept
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.2f", value);
        panel().children()[1].clientPanel().template as<TextEntry>()
            .setText(buf);
    }

private:
    [[nodiscard]] decltype(auto) panel() const noexcept
    {
        return hookContext.template make<PanoramaUiPanel>(_panel);
    }

    HookContext& hookContext;
    cs2::CUIPanel* _panel;
};
