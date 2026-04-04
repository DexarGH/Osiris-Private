#pragma once

#include <cstdint>

#include <Config/ConfigVariable.h>
#include <Config/RangeConstrainedVariableParams.h>

namespace trigger_bot
{

enum class BindMode : std::uint8_t {
    Hold,
    Toggle,
    AlwaysOn
};

constexpr auto kBindIndex = RangeConstrainedVariableParams<std::uint8_t>{
    .min = 0,
    .max = 108,
    .def = 0
};

constexpr auto kDelayParams = RangeConstrainedVariableParams<std::uint16_t>{
    .min = 0,
    .max = 999,
    .def = 0
};

CONFIG_VARIABLE(Enabled, bool, false);
CONFIG_VARIABLE_RANGE(Bind, kBindIndex);
CONFIG_VARIABLE(BindModeType, BindMode, BindMode::Hold);
CONFIG_VARIABLE_RANGE(MinDelay, kDelayParams);
CONFIG_VARIABLE_RANGE(MaxDelay, kDelayParams);
CONFIG_VARIABLE(HeadOnly, bool, false);

}
