#pragma once

#include <concepts>

#include <Features/Aimbot/AimbotConfigVariables.h>
#include <Features/Combat/SniperRifles/NoScopeInaccuracyVis/NoScopeInaccuracyVisConfigVariables.h>
#include <Features/Hud/FovCircle/FovCircleConfigVariables.h>
#include <Features/Visuals/Rain/RainConfigVariables.h>
#include <Features/Visuals/Tail/TailConfigVariables.h>
#include <Features/Visuals/PlayerInfoInWorld/PlayerInfoInWorld.h>
#include <GameClient/Panorama/Slider.h>
#include <GameClient/Panorama/TextEntry.h>
#include <Platform/Macros/FunctionAttributes.h>
#include <Utils/StringParser.h>
#include "Tabs/VisualsTab/HueSlider.h"
#include "Tabs/VisualsTab/IntSlider.h"

template <typename HookContext>
struct SetCommandHandler {
    SetCommandHandler(StringParser& parser, HookContext& hookContext) noexcept
        : parser{parser}
        , hookContext{hookContext}
    {
    }

    void operator()() noexcept
    {
        if (const auto section = parser.getLine('/'); section == "combat") {
            handleCombatSection();
        } else if (section == "hud") {
            handleHudSection();
        } else if (section == "visuals") {
            handleVisualsSection();
        } else if (section == "sound") {
            handleSoundSection();
        }
    }

private:
    void handleCombatSection() const noexcept
    {
        if (const auto feature = parser.getLine('/'); feature == "aimbot_smooth") {
            handleIntSlider<aimbot::Smooth>("aimbot_smooth");
        } else if (feature == "aimbot_smooth_text") {
            handleIntSliderTextEntry<aimbot::Smooth>("aimbot_smooth");
        } else if (feature == "aimbot_fov") {
            handleIntSlider<aimbot::Fov>("aimbot_fov");
        } else if (feature == "aimbot_fov_text") {
            handleIntSliderTextEntry<aimbot::Fov>("aimbot_fov");
        } else if (feature == "aimbot_multi_point_size") {
            handleIntSlider<aimbot::MultiPointSize>("aimbot_multi_point_size");
        } else if (feature == "aimbot_multi_point_size_text") {
            handleIntSliderTextEntry<aimbot::MultiPointSize>("aimbot_multi_point_size");
        }
    }

    void handleHudSection() const noexcept
    {
        if (const auto feature = parser.getLine('/'); feature == "fov_circle_thickness") {
            handleIntSlider<fov_circle_vars::Thickness>("fov_circle_thickness");
        } else if (feature == "fov_circle_thickness_text") {
            handleIntSliderTextEntry<fov_circle_vars::Thickness>("fov_circle_thickness");
        }
    }

    void handleSoundSection() const noexcept
    {
    }

    void handleVisualsSection() const noexcept
    {
        if (const auto feature = parser.getLine('/'); feature == "viewmodel_fov") {
            handleIntSlider<viewmodel_mod_vars::Fov>("viewmodel_fov");
        } else if (feature == "viewmodel_fov_text") {
            handleIntSliderTextEntry<viewmodel_mod_vars::Fov>("viewmodel_fov");
        } else if (feature == "rain_count") {
            handleIntSlider<rain_vars::Count>("rain_count");
        } else if (feature == "rain_count_text") {
            handleIntSliderTextEntry<rain_vars::Count>("rain_count");
        } else if (feature == "rain_color_hue") {
            handleHueSlider<rain_vars::ColorHue>("rain_color_hue");
        } else if (feature == "rain_color_hue_text") {
            handleHueSliderTextEntry<rain_vars::ColorHue>("rain_color_hue");
        } else if (feature == "tail_count") {
            handleIntSlider<tail_vars::Count>("tail_count");
        } else if (feature == "tail_count_text") {
            handleIntSliderTextEntry<tail_vars::Count>("tail_count");
        } else if (feature == "tail_color_hue") {
            handleHueSlider<tail_vars::ColorHue>("tail_color_hue");
        } else if (feature == "tail_color_hue_text") {
            handleHueSliderTextEntry<tail_vars::ColorHue>("tail_color_hue");
        }
    }

    template <typename ConfigVariable>
    void handleIntSlider(const char* sliderId) const noexcept
    {
        using ValueType = typename ConfigVariable::ValueType::ValueType;
        const auto newVariableValue = handleIntSlider(sliderId, ConfigVariable::ValueType::kMin, ConfigVariable::ValueType::kMax, static_cast<ValueType>(GET_CONFIG_VAR(ConfigVariable)));
        hookContext.config().template setVariable<ConfigVariable>(typename ConfigVariable::ValueType{newVariableValue});
    }

    template <std::unsigned_integral IntegerType>
    [[nodiscard]] IntegerType handleIntSlider(const char* sliderId, IntegerType min, IntegerType max, IntegerType current) const noexcept
    {
        IntegerType value{};
        if (!parser.parseInt(value) || value == current || value < min || value > max)
            return current;

        auto&& slider = getIntSlider(sliderId);
        slider.updateTextEntry(value);
        return value;
    }

    template <typename ConfigVariable>
    void handleIntSliderTextEntry(const char* sliderId) const noexcept
    {
        using ValueType = typename ConfigVariable::ValueType::ValueType;
        const auto newVariableValue = handleIntSliderTextEntry(sliderId, ConfigVariable::ValueType::kMin, ConfigVariable::ValueType::kMax, static_cast<ValueType>(GET_CONFIG_VAR(ConfigVariable)));
        hookContext.config().template setVariable<ConfigVariable>(typename ConfigVariable::ValueType{newVariableValue});
    }

    template <std::unsigned_integral IntegerType>
    [[nodiscard]] IntegerType handleIntSliderTextEntry(const char* sliderId, IntegerType min, IntegerType max, IntegerType current) const noexcept
    {
        auto&& slider = getIntSlider(sliderId);
        IntegerType value{};
        if (!parser.parseInt(value) || value < min || value > max) {
            slider.updateTextEntry(current);
            return current;
        }
        slider.updateTextEntry(value);
        return value;
    }

    [[nodiscard]] auto getHueSlider(const char* sliderId) const noexcept
    {
        const auto mainMenuPointer = hookContext.patternSearchResults().template get<MainMenuPanelPointer>();
        auto&& mainMenu = hookContext.template make<ClientPanel>(mainMenuPointer ? *mainMenuPointer : nullptr).uiPanel();
        return hookContext.template make<HueSlider>(mainMenu.findChildInLayoutFile(sliderId));
    }

    [[nodiscard]] auto getIntSlider(const char* sliderId) const noexcept
    {
        const auto mainMenuPointer = hookContext.patternSearchResults().template get<MainMenuPanelPointer>();
        auto&& mainMenu = hookContext.template make<ClientPanel>(mainMenuPointer ? *mainMenuPointer : nullptr).uiPanel();
        return hookContext.template make<IntSlider>(mainMenu.findChildInLayoutFile(sliderId));
    }

    template <typename ConfigVariable>
    void handleHueSlider(const char* sliderId) const noexcept
    {
        using ValueType = typename ConfigVariable::ValueType::ValueType;
        const auto newVariableValue = handleIntSlider(sliderId, ConfigVariable::ValueType::kMin, ConfigVariable::ValueType::kMax, static_cast<ValueType>(GET_CONFIG_VAR(ConfigVariable)));
        hookContext.config().template setVariable<ConfigVariable>(typename ConfigVariable::ValueType{newVariableValue});

        auto&& slider = getHueSlider(sliderId);
        slider.updateTextEntry(color::HueInteger{newVariableValue});
        slider.updateColorPreview(color::HueInteger{newVariableValue});
    }

    template <typename ConfigVariable>
    void handleHueSliderTextEntry(const char* sliderId) const noexcept
    {
        using ValueType = typename ConfigVariable::ValueType::ValueType;
        const auto newVariableValue = handleIntSliderTextEntry(sliderId, ConfigVariable::ValueType::kMin, ConfigVariable::ValueType::kMax, static_cast<ValueType>(GET_CONFIG_VAR(ConfigVariable)));
        hookContext.config().template setVariable<ConfigVariable>(typename ConfigVariable::ValueType{newVariableValue});

        auto&& slider = getHueSlider(sliderId);
        slider.updateTextEntry(color::HueInteger{newVariableValue});
        slider.updateColorPreview(color::HueInteger{newVariableValue});
    }

    StringParser& parser;
    HookContext& hookContext;
};
