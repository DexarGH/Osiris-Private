#pragma once

#include <cstdint>

#include <Config/ConfigVariable.h>
#include <Config/RangeConstrainedVariableParams.h>

#include "AimbotActivationKeys.h"
#include "AimbotParams.h"

namespace aimbot
{

enum class BindMode : std::uint8_t {
    Hold,
    Toggle,
    AlwaysOn
};

enum class RotationType : std::uint8_t {
    Linear,
    Sigmoid
};

constexpr auto kBindIndex = RangeConstrainedVariableParams<std::uint8_t>{
    .min = 0,
    .max = static_cast<std::uint8_t>(kActivationKeyBindings.size() - 1),
    .def = 0
};

constexpr auto kSmoothParams = RangeConstrainedVariableParams<std::uint16_t>{
    .min = 0,
    .max = 20,
    .def = 5
};

constexpr auto kFovParams = RangeConstrainedVariableParams<std::uint16_t>{
    .min = 0,
    .max = 180,
    .def = 10
};

constexpr auto kMultiPointSizeParams = RangeConstrainedVariableParams<std::uint16_t>{
    .min = 0,
    .max = 100,
    .def = 50
};

CONFIG_VARIABLE(Enabled, bool, true);
CONFIG_VARIABLE_RANGE(Bind, kBindIndex);
CONFIG_VARIABLE(BindModeType, BindMode, BindMode::Hold);
CONFIG_VARIABLE_RANGE(Smooth, kSmoothParams);
CONFIG_VARIABLE(Rotation, RotationType, RotationType::Linear);
CONFIG_VARIABLE_RANGE(Fov, kFovParams);
CONFIG_VARIABLE_RANGE(MultiPointSize, kMultiPointSizeParams);
CONFIG_VARIABLE(VisibleChecks, bool, true);
CONFIG_VARIABLE(FlashChecks, bool, true);

}
