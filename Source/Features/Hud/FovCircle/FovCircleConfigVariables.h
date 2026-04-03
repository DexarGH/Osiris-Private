#pragma once

#include <cstdint>

#include <Config/ConfigVariable.h>
#include <Config/RangeConstrainedVariableParams.h>

namespace fov_circle_vars
{

enum class ColorType : std::uint8_t {
    Green,
    Red,
    Blue,
    Yellow,
    Cyan,
    Magenta,
    White
};

constexpr auto kThicknessParams = RangeConstrainedVariableParams<std::uint16_t>{
    .min = 1,
    .max = 10,
    .def = 2
};

CONFIG_VARIABLE(Enabled, bool, false);
CONFIG_VARIABLE_RANGE(Thickness, kThicknessParams);
CONFIG_VARIABLE(Color, ColorType, ColorType::Green);

}
