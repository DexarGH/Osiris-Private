#pragma once

#include <cstdint>

#include <Config/ConfigVariable.h>
#include <Config/RangeConstrainedVariableParams.h>
#include <Utils/ColorUtils.h>
#include <Utils/InRange.h>

namespace rain_vars
{

enum class ParticleType : std::uint8_t {
    Star
};

constexpr auto kCountParams = RangeConstrainedVariableParams<std::uint16_t>{
    .min = 1,
    .max = 500,
    .def = 50
};

constexpr auto kSpawnRadiusParams = RangeConstrainedVariableParams<std::uint16_t>{
    .min = 100,
    .max = 2000,
    .def = 500
};

constexpr auto kTiltAngleParams = RangeConstrainedVariableParams<std::uint16_t>{
    .min = 0,
    .max = 90,
    .def = 0
};

constexpr auto kSpeedParams = RangeConstrainedVariableParams<std::uint16_t>{
    .min = 1,
    .max = 500,
    .def = 100
};

constexpr auto kLiveTimeParams = RangeConstrainedVariableParams<std::uint16_t>{
    .min = 1,
    .max = 20,
    .def = 5
};

using ColorHueType = InRange<color::HueInteger::UnderlyingType, 0, 360>;
constexpr ColorHueType kColorHueDef{120};

CONFIG_VARIABLE(Enabled, bool, false);
CONFIG_VARIABLE_RANGE(Count, kCountParams);
CONFIG_VARIABLE_RANGE(SpawnRadius, kSpawnRadiusParams);
CONFIG_VARIABLE_RANGE(TiltAngle, kTiltAngleParams);
CONFIG_VARIABLE_RANGE(Speed, kSpeedParams);
CONFIG_VARIABLE_RANGE(LiveTime, kLiveTimeParams);
CONFIG_VARIABLE(Type, ParticleType, ParticleType::Star);
CONFIG_VARIABLE(ColorHue, ColorHueType, kColorHueDef);

}
