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
        switch (selectedIndex) {
        case 0: SET_CONFIG_VAR(world_particle_vars::Type, world_particle_vars::ParticleType::Star); break;
        case 1: SET_CONFIG_VAR(world_particle_vars::Type, world_particle_vars::ParticleType::Snow); break;
        case 2: SET_CONFIG_VAR(world_particle_vars::Type, world_particle_vars::ParticleType::Bloom); break;
        case 3: SET_CONFIG_VAR(world_particle_vars::Type, world_particle_vars::ParticleType::Dollar); break;
        default: break;
        }
    }

private:
    HookContext& hookContext;
};
