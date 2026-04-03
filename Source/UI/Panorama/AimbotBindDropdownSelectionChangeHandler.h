#pragma once

#include <cstdint>

#include <Features/Aimbot/AimbotConfigVariables.h>
#include <HookContext/HookContextMacros.h>

template <typename HookContext>
struct AimbotBindDropdownSelectionChangeHandler {
    explicit AimbotBindDropdownSelectionChangeHandler(HookContext& hookContext) noexcept
        : hookContext{hookContext}
    {
    }

    void onSelectionChanged(int selectedIndex)
    {
        if (selectedIndex < 0 || selectedIndex >= static_cast<int>(aimbot::kActivationKeyBindings.size()))
            return;

        hookContext.config().template setVariable<aimbot::Bind>(typename aimbot::Bind::ValueType{static_cast<std::uint8_t>(selectedIndex)});
    }

private:
    HookContext& hookContext;
};
