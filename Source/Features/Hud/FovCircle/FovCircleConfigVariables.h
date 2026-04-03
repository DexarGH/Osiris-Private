#pragma once

#include <cstdint>

#include <Config/ConfigVariable.h>

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

CONFIG_VARIABLE(Enabled, bool, false);
CONFIG_VARIABLE(Fov, std::uint16_t, 90);
CONFIG_VARIABLE(Thickness, std::uint8_t, 2);
CONFIG_VARIABLE(Color, ColorType, ColorType::Green);

}
