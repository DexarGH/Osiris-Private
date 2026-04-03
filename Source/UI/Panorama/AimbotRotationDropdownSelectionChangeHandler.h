#pragma once

#include <Features/Aimbot/AimbotConfigVariables.h>
#include <HookContext/HookContextMacros.h>

template <typename HookContext>
struct AimbotRotationDropdownSelectionChangeHandler {
    explicit AimbotRotationDropdownSelectionChangeHandler(HookContext& hookContext) noexcept
        : hookContext{hookContext}
    {
    }

    void onSelectionChanged(int selectedIndex)
    {
        switch (selectedIndex) {
        case 0: SET_CONFIG_VAR(aimbot::Rotation, aimbot::RotationType::Linear); break;
        case 1: SET_CONFIG_VAR(aimbot::Rotation, aimbot::RotationType::Sigmoid); break;
        default: break;
        }
    }

private:
    HookContext& hookContext;
};
