#pragma once

#include <BuildConfig.h>

#if IS_LINUX()
#include <Platform/Linux/LinuxInput.h>
using InputType = LinuxInput;
#elif IS_WIN64()
#include <Platform/Windows/WindowsInput.h>
using InputType = WindowsInput;
#else
struct InputType {
    void moveMouseRelative(float, float) noexcept {}
    void simulateAttack() noexcept {}
    bool isKeyboardKeyPressed(std::uint16_t) const noexcept { return false; }
    bool getMouseButtonState(std::uint8_t) const noexcept { return false; }
    void updateKeyboardState(const bool*) noexcept {}
    void updateMouseButtonState(std::uint8_t, bool) noexcept {}
};
#endif
