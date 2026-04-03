#pragma once

#include <concepts>

#include <Features/Aimbot/AimbotConfigVariables.h>
#include <Features/Combat/SniperRifles/NoScopeInaccuracyVis/NoScopeInaccuracyVisConfigVariables.h>
#include <Features/Hud/FovCircle/FovCircleConfigVariables.h>
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
        if (const auto feature = parser.getLine('/'); feature == "fov_circle_fov") {
            handleSimpleIntSlider<fov_circle_vars::Fov>("fov_circle_fov", 0, 180);
        } else if (feature == "fov_circle_fov_text") {
            handleSimpleIntSliderTextEntry<fov_circle_vars::Fov>("fov_circle_fov", 0, 180);
        } else if (feature == "fov_circle_thickness") {
            handleSimpleIntSlider<fov_circle_vars::Thickness>("fov_circle_thickness", 1, 10);
        } else if (feature == "fov_circle_thickness_text") {
            handleSimpleIntSliderTextEntry<fov_circle_vars::Thickness>("fov_circle_thickness", 1, 10);
        }
    }

    template <typename ConfigVariable>
    void handleSimpleIntSlider(const char* sliderId, auto min, auto max) const noexcept
    {
        using ValueType = typename ConfigVariable::ValueType;
        const auto current = static_cast<ValueType>(GET_CONFIG_VAR(ConfigVariable));
        ValueType value{};
        if (!parser.parseInt(value) || value == current || value < static_cast<ValueType>(min) || value > static_cast<ValueType>(max))
            return;

        hookContext.config().template setVariable<ConfigVariable>(value);
    }

    template <typename ConfigVariable>
    void handleSimpleIntSliderTextEntry(const char* sliderId, auto min, auto max) const noexcept
    {
        using ValueType = typename ConfigVariable::ValueType;
        const auto current = static_cast<ValueType>(GET_CONFIG_VAR(ConfigVariable));
        ValueType value{};
        if (!parser.parseInt(value) || value < static_cast<ValueType>(min) || value > static_cast<ValueType>(max)) {
            updateSliderText(sliderId, current);
            return;
        }

        if (value == current)
            return;

        hookContext.config().template setVariable<ConfigVariable>(value);
    }

    void updateSliderText(const char* sliderId, auto value) const noexcept
    {
        // Update slider text entry via GUI command
        static_cast<void>(sliderId);
        static_cast<void>(value);
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

        if (value == current)
            return current;

        slider.updateSlider(value);
        return value;
    }

    [[nodiscard]] decltype(auto) getIntSlider(const char* sliderId) const noexcept
    {
        const auto mainMenuPointer = hookContext.patternSearchResults().template get<MainMenuPanelPointer>();
        auto&& mainMenu = hookContext.template make<ClientPanel>(mainMenuPointer ? *mainMenuPointer : nullptr).uiPanel();
        return hookContext.template make<IntSlider>(mainMenu.findChildInLayoutFile(sliderId));
    }

    StringParser& parser;
    HookContext& hookContext;
};
