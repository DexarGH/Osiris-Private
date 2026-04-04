#pragma once

template <typename HookContext>
struct RainParticleTypeDropdownSelectionChangeHandler {
    void onSelectionChanged(std::uint8_t selectedIndex) const noexcept
    {
        hookContext.config().template setVariable<rain_vars::Type>(static_cast<rain_vars::ParticleType>(selectedIndex));
    }

    HookContext& hookContext;
};
