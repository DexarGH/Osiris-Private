#pragma once

#include <Platform/Macros/IsPlatform.h>

#if IS_WIN64()

#include <windows.h>

#include "WindowsDynamicLibrary.h"

#endif

class WindowsInput {
public:
    void moveMouseRelative(float deltaX, float deltaY) noexcept
    {
#if IS_WIN64()
        if (!sendInput)
            sendInput = WindowsDynamicLibrary{"user32.dll"}.getFunctionAddress("SendInput").as<decltype(&::SendInput)>();
        if (!sendInput)
            return;

        subPixelRemainderX += deltaX;
        subPixelRemainderY += deltaY;

        const auto integerDeltaX = roundToInt(subPixelRemainderX);
        const auto integerDeltaY = roundToInt(subPixelRemainderY);
        subPixelRemainderX -= static_cast<float>(integerDeltaX);
        subPixelRemainderY -= static_cast<float>(integerDeltaY);

        if (integerDeltaX == 0 && integerDeltaY == 0)
            return;

        INPUT input{};
        input.type = INPUT_MOUSE;
        input.mi.dx = integerDeltaX;
        input.mi.dy = integerDeltaY;
        input.mi.dwFlags = MOUSEEVENTF_MOVE;
        sendInput(1, &input, sizeof(INPUT));
#else
        static_cast<void>(deltaX);
        static_cast<void>(deltaY);
#endif
    }

    void simulateAttack() noexcept
    {
#if IS_WIN64()
        if (!sendInput)
            sendInput = WindowsDynamicLibrary{"user32.dll"}.getFunctionAddress("SendInput").as<decltype(&::SendInput)>();
        if (!sendInput)
            return;

        INPUT press{};
        press.type = INPUT_MOUSE;
        press.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        sendInput(1, &press, sizeof(INPUT));

        INPUT release{};
        release.type = INPUT_MOUSE;
        release.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        sendInput(1, &release, sizeof(INPUT));
#endif
    }

    [[nodiscard]] bool isKeyboardKeyPressed(std::uint16_t code) const noexcept
    {
#if IS_WIN64()
        return keyboardStates[code];
#else
        static_cast<void>(code);
        return false;
#endif
    }

    [[nodiscard]] bool getMouseButtonState(std::uint8_t button) const noexcept
    {
#if IS_WIN64()
        return mouseButtonStates[button];
#else
        static_cast<void>(button);
        return false;
#endif
    }

    void updateKeyboardState(const bool* states) noexcept
    {
#if IS_WIN64()
        if (states) {
            for (std::size_t i = 0; i < 256; ++i)
                keyboardStates[i] = states[i];
        }
#else
        static_cast<void>(states);
#endif
    }

    void updateMouseButtonState(std::uint8_t button, bool pressed) noexcept
    {
#if IS_WIN64()
        if (button < 8)
            mouseButtonStates[button] = pressed;
#else
        static_cast<void>(button);
        static_cast<void>(pressed);
#endif
    }

private:
#if IS_WIN64()
    [[nodiscard]] static int roundToInt(float value) noexcept
    {
        return static_cast<int>(value >= 0.0f ? value + 0.5f : value - 0.5f);
    }

    decltype(&::SendInput) sendInput{
        WindowsDynamicLibrary{"user32.dll"}.getFunctionAddress("SendInput").as<decltype(&::SendInput)>()
    };
    float subPixelRemainderX{0.0f};
    float subPixelRemainderY{0.0f};
#endif
    bool keyboardStates[256]{};
    bool mouseButtonStates[8]{};
};
