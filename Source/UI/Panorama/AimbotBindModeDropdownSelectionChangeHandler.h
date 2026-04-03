#pragma once

#include <Features/Aimbot/AimbotConfigVariables.h>
#include <HookContext/HookContextMacros.h>

template <typename HookContext>
struct AimbotBindModeDropdownSelectionChangeHandler {
    explicit AimbotBindModeDropdownSelectionChangeHandler(HookContext& hookContext) noexcept
        : hookContext{hookContext}
    {
    }

    void onSelectionChanged(int selectedIndex)
    {
        switch (selectedIndex) {
        case 0: SET_CONFIG_VAR(aimbot::BindModeType, aimbot::BindMode::Hold); break;
        case 1: SET_CONFIG_VAR(aimbot::BindModeType, aimbot::BindMode::Toggle); break;
        case 2: SET_CONFIG_VAR(aimbot::BindModeType, aimbot::BindMode::AlwaysOn); break;
        default: break;
        }
    }

private:
    HookContext& hookContext;
};
