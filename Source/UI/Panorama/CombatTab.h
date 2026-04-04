#pragma once

#include <concepts>
#include <cstdint>

#include <GameClient/Panorama/PanoramaDropDown.h>
#include <EntryPoints/GuiEntryPoints.h>
#include <Platform/Macros/FunctionAttributes.h>

#include "Tabs/VisualsTab/IntSlider.h"
#include "AimbotBindDropdownSelectionChangeHandler.h"
#include "AimbotBindModeDropdownSelectionChangeHandler.h"
#include "AimbotRotationDropdownSelectionChangeHandler.h"
#include "OnOffDropdownSelectionChangeHandler.h"
#include "Tabs/CombatTab/TriggerBotBindDropdownSelectionChangeHandler.h"
#include "Tabs/CombatTab/TriggerBotBindModeDropdownSelectionChangeHandler.h"

template <typename HookContext>
class CombatTab {
public:
    explicit CombatTab(HookContext& hookContext) noexcept
        : hookContext{hookContext}
    {
    }

    void init(auto&& guiPanel) const
    {
        initDropDown<OnOffDropdownSelectionChangeHandler<HookContext, aimbot::Enabled>>(guiPanel, "aimbot_enabled");
        initDropDown<AimbotBindDropdownSelectionChangeHandler<HookContext>>(guiPanel, "aimbot_bind");
        initDropDown<AimbotBindModeDropdownSelectionChangeHandler<HookContext>>(guiPanel, "aimbot_bind_mode");
        initDropDown<AimbotRotationDropdownSelectionChangeHandler<HookContext>>(guiPanel, "aimbot_rotation");
        initDropDown<OnOffDropdownSelectionChangeHandler<HookContext, aimbot::VisibleChecks>>(guiPanel, "aimbot_visible_checks");
        initDropDown<OnOffDropdownSelectionChangeHandler<HookContext, aimbot::FlashChecks>>(guiPanel, "aimbot_flash_checks");
        initDropDown<OnOffDropdownSelectionChangeHandler<HookContext, no_scope_inaccuracy_vis_vars::Enabled>>(guiPanel, "no_scope_inacc_vis");

        initDropDown<OnOffDropdownSelectionChangeHandler<HookContext, trigger_bot::Enabled>>(guiPanel, "triggerbot_enabled");
        initDropDown<TriggerBotBindDropdownSelectionChangeHandler<HookContext>>(guiPanel, "triggerbot_bind");
        initDropDown<TriggerBotBindModeDropdownSelectionChangeHandler<HookContext>>(guiPanel, "triggerbot_bind_mode");
        initDropDown<OnOffDropdownSelectionChangeHandler<HookContext, trigger_bot::HeadOnly>>(guiPanel, "triggerbot_head_only");
    }

    void updateFromConfig(auto&& mainMenu) const noexcept
    {
        setDropDownSelectedIndex(mainMenu, "aimbot_enabled", !GET_CONFIG_VAR(aimbot::Enabled));
        setDropDownSelectedIndex(mainMenu, "aimbot_bind", static_cast<int>(static_cast<std::uint8_t>(GET_CONFIG_VAR(aimbot::Bind))));
        setDropDownSelectedIndex(mainMenu, "aimbot_bind_mode", static_cast<int>(GET_CONFIG_VAR(aimbot::BindModeType)));
        updateSlider<aimbot::Smooth>(mainMenu, "aimbot_smooth");
        setDropDownSelectedIndex(mainMenu, "aimbot_rotation", static_cast<int>(GET_CONFIG_VAR(aimbot::Rotation)));
        updateSlider<aimbot::Fov>(mainMenu, "aimbot_fov");
        updateSlider<aimbot::MultiPointSize>(mainMenu, "aimbot_multi_point_size");
        setDropDownSelectedIndex(mainMenu, "aimbot_visible_checks", !GET_CONFIG_VAR(aimbot::VisibleChecks));
        setDropDownSelectedIndex(mainMenu, "aimbot_flash_checks", !GET_CONFIG_VAR(aimbot::FlashChecks));
        setDropDownSelectedIndex(mainMenu, "no_scope_inacc_vis", !GET_CONFIG_VAR(no_scope_inaccuracy_vis_vars::Enabled));

        setDropDownSelectedIndex(mainMenu, "triggerbot_enabled", !GET_CONFIG_VAR(trigger_bot::Enabled));
        setDropDownSelectedIndex(mainMenu, "triggerbot_bind", static_cast<int>(static_cast<std::uint8_t>(GET_CONFIG_VAR(trigger_bot::Bind))));
        setDropDownSelectedIndex(mainMenu, "triggerbot_bind_mode", static_cast<int>(GET_CONFIG_VAR(trigger_bot::BindModeType)));
        updateSlider<trigger_bot::MinDelay>(mainMenu, "triggerbot_min_delay");
        updateSlider<trigger_bot::MaxDelay>(mainMenu, "triggerbot_max_delay");
        setDropDownSelectedIndex(mainMenu, "triggerbot_head_only", !GET_CONFIG_VAR(trigger_bot::HeadOnly));
    }

private:
    template <typename Handler>
    void initDropDown(auto&& guiPanel, const char* panelId) const
    {
        auto&& dropDown = guiPanel.findChildInLayoutFile(panelId).clientPanel().template as<PanoramaDropDown>();
        dropDown.registerSelectionChangedHandler(&GuiEntryPoints<HookContext>::template dropDownSelectionChanged<Handler>);
    }

    [[NOINLINE]] void setDropDownSelectedIndex(auto&& mainMenu, const char* dropDownId, int selectedIndex) const noexcept
    {
        mainMenu.findChildInLayoutFile(dropDownId).clientPanel().template as<PanoramaDropDown>().setSelectedIndex(selectedIndex);
    }

    template <typename ConfigVariable>
    void updateSlider(auto&& mainMenu, const char* sliderId) const noexcept
    {
        const auto value = static_cast<std::uint16_t>(GET_CONFIG_VAR(ConfigVariable));
        auto&& sliderPanel = mainMenu.findChildInLayoutFile(sliderId);
        if (!sliderPanel)
            return;
        auto&& slider = hookContext.template make<IntSlider>(sliderPanel);
        slider.updateSlider(value);
        slider.updateTextEntry(value);
    }

    template <std::unsigned_integral IntegerType>
    void updateSlider(auto&& mainMenu, const char* sliderId, IntegerType value) const noexcept
    {
        auto&& sliderPanel = mainMenu.findChildInLayoutFile(sliderId);
        if (!sliderPanel)
            return;
        auto&& slider = hookContext.template make<IntSlider>(sliderPanel);
        slider.updateSlider(value);
        slider.updateTextEntry(value);
    }

    HookContext& hookContext;
};
