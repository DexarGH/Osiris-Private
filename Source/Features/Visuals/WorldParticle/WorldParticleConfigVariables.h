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

constexpr auto kColorRParams = RangeConstrainedVariableParams<std::uint16_t>{
    .min = 0,
    .max = 255,
    .def = 255
};

constexpr auto kColorGParams = RangeConstrainedVariableParams<std::uint16_t>{
    .min = 0,
    .max = 255,
    .def = 255
};

constexpr auto kColorBParams = RangeConstrainedVariableParams<std::uint16_t>{
    .min = 0,
    .max = 255,
    .def = 255
};

CONFIG_VARIABLE(Enabled, bool, false);
CONFIG_VARIABLE_RANGE(Count, kCountParams);
CONFIG_VARIABLE(ModeType, Mode, Mode::Rain);
CONFIG_VARIABLE(Type, ParticleType, ParticleType::Snow);
CONFIG_VARIABLE_RANGE(ColorR, kColorRParams);
CONFIG_VARIABLE_RANGE(ColorG, kColorGParams);
CONFIG_VARIABLE_RANGE(ColorB, kColorBParams);

}
