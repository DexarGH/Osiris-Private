#pragma once

#include <cstdint>

#include <Config/ConfigVariable.h>
#include <Config/RangeConstrainedVariableParams.h>
#include <Utils/ColorUtils.h>
#include <Utils/InRange.h>

namespace tail_vars
{

enum class ParticleType : std::uint8_t {
    Star
};

constexpr auto kCountParams = RangeConstrainedVariableParams<std::uint16_t>{
    .min = 1,
    .max = 500,
    .def = 50
};

using ColorHueType = InRange<color::HueInteger::UnderlyingType, 0, 360>;
constexpr ColorHueType kColorHueDef{120};

CONFIG_VARIABLE(Enabled, bool, false);
CONFIG_VARIABLE_RANGE(Count, kCountParams);
CONFIG_VARIABLE(Type, ParticleType, ParticleType::Star);
CONFIG_VARIABLE(ColorHue, ColorHueType, kColorHueDef);

}
