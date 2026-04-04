#pragma once

#include <cstdint>

#include <Features/Aimbot/AimbotActivationKeys.h>
#include <Features/TriggerBot/TriggerBotConfigVariables.h>
#include <HookContext/HookContextMacros.h>

template <typename HookContext>
struct TriggerBotBindDropdownSelectionChangeHandler {
    explicit TriggerBotBindDropdownSelectionChangeHandler(HookContext& hookContext) noexcept
        : hookContext{hookContext}
    {
    }

    void onSelectionChanged(int selectedIndex)
    {
        if (selectedIndex < 0 || selectedIndex >= static_cast<int>(aimbot::kActivationKeyBindings.size()))
            return;

        hookContext.config().template setVariable<trigger_bot::Bind>(typename trigger_bot::Bind::ValueType{static_cast<std::uint8_t>(selectedIndex)});
    }

private:
    HookContext& hookContext;
};
