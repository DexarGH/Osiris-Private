#pragma once

#include <cstdint>

#include <Config/RangeConstrainedVariableParams.h>

namespace aimbot_params
{

[[nodiscard]] constexpr float multiPointSizeFromSlider(std::uint16_t value) noexcept
{
    return static_cast<float>(value) * 0.01f;
}

}
