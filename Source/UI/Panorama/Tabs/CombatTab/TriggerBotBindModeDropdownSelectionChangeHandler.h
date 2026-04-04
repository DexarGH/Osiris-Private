#pragma once

#include <cstdint>

#include <Features/TriggerBot/TriggerBotConfigVariables.h>
#include <HookContext/HookContextMacros.h>

template <typename HookContext>
struct TriggerBotBindModeDropdownSelectionChangeHandler {
    explicit TriggerBotBindModeDropdownSelectionChangeHandler(HookContext& hookContext) noexcept
        : hookContext{hookContext}
    {
    }

    void onSelectionChanged(int selectedIndex)
    {
        switch (selectedIndex) {
        case 0: hookContext.config().template setVariable<trigger_bot::BindModeType>(trigger_bot::BindMode::Hold); break;
        case 1: hookContext.config().template setVariable<trigger_bot::BindModeType>(trigger_bot::BindMode::Toggle); break;
        case 2: hookContext.config().template setVariable<trigger_bot::BindModeType>(trigger_bot::BindMode::AlwaysOn); break;
        default: break;
        }
    }

private:
    HookContext& hookContext;
};
