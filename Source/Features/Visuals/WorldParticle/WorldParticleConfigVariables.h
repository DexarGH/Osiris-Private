#pragma once

#include <cstdint>

#include <Config/ConfigVariable.h>
#include <Config/RangeConstrainedVariableParams.h>

namespace world_particle_vars
{

enum class Mode : std::uint8_t {
    Rain,
    Tail
};

enum class ParticleType : std::uint8_t {
    Star,
    Snow,
    Bloom,
    Dollar
};

constexpr auto kCountParams = RangeConstrainedVariableParams<std::uint16_t>{
    .min = 10,
    .max = 1000,
    .def = 100
};

CONFIG_VARIABLE(Enabled, bool, false);
CONFIG_VARIABLE_RANGE(Count, kCountParams);
CONFIG_VARIABLE(ModeType, Mode, Mode::Rain);
CONFIG_VARIABLE(Type, ParticleType, ParticleType::Snow);
CONFIG_VARIABLE(ColorR, std::uint8_t, 255);
CONFIG_VARIABLE(ColorG, std::uint8_t, 255);
CONFIG_VARIABLE(ColorB, std::uint8_t, 255);

}
