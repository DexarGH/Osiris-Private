#pragma once

#include <Features/Visuals/WorldParticle/WorldParticleConfigVariables.h>
#include <HookContext/HookContextMacros.h>

template <typename HookContext>
struct WorldParticleModeDropdownSelectionChangeHandler {
    explicit WorldParticleModeDropdownSelectionChangeHandler(HookContext& hookContext) noexcept
        : hookContext{hookContext}
    {
    }

    void onSelectionChanged(int selectedIndex)
    {
        switch (selectedIndex) {
        case 0: SET_CONFIG_VAR(world_particle_vars::ModeType, world_particle_vars::Mode::Rain); break;
        case 1: SET_CONFIG_VAR(world_particle_vars::ModeType, world_particle_vars::Mode::Tail); break;
        default: break;
        }
    }

private:
    HookContext& hookContext;
};
