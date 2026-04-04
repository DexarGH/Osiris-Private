#pragma once

template <typename HookContext>
struct TailParticleTypeDropdownSelectionChangeHandler {
    void onSelectionChanged(std::uint8_t selectedIndex) const noexcept
    {
        hookContext.config().template setVariable<tail_vars::Type>(static_cast<tail_vars::ParticleType>(selectedIndex));
    }

    HookContext& hookContext;
};
