#pragma once

#include <Features/Visuals/WorldParticle/WorldParticleConfigVariables.h>
#include <HookContext/HookContextMacros.h>

template <typename HookContext>
struct WorldParticleTypeDropdownSelectionChangeHandler {
    explicit WorldParticleTypeDropdownSelectionChangeHandler(HookContext& hookContext) noexcept
        : hookContext{hookContext}
    {
    }

    void onSelectionChanged(int selectedIndex)
    {
        // Заготовка для будущих типов — сейчас только Star
        switch (selectedIndex) {
        case 0: SET_CONFIG_VAR(world_particle_vars::Type, world_particle_vars::ParticleType::Star); break;
        default: break;
        }
    }

private:
    HookContext& hookContext;
};
