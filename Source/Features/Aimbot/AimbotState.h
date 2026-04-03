#pragma once

#include <cstdint>

#include <Platform/Macros/IsPlatform.h>
#include <Platform/Linux/LinuxInput.h>
#include <Platform/Windows/WindowsInput.h>
#if IS_WIN64()
#include <windows.h>
#endif
#include <SDL/SdlFunctions.h>

struct AimbotState {
    LinuxInput linuxInput;
    WindowsInput windowsInput;
    sdl3::SDL_GetKeyboardState* getKeyboardState{nullptr};
    sdl3::SDL_GetMouseState* getMouseState{nullptr};
    int selfMemFd{-1};
    bool processVmReadvUnavailable{false};
    bool hasResolvedBoneArrayPointerFieldOffset{false};
    std::uint16_t resolvedBoneArrayPointerFieldOffset{0};
    std::uint8_t boneArrayPointerFieldRescanCooldown{0};
    int debugLogFd{-1};
    std::uint32_t debugLogCounter{0};
    // Toggle state for BindMode
    bool toggleState{false};
    bool lastBindWasPressed{false};
#if IS_WIN64()
    decltype(&::ReadProcessMemory) readProcessMemory{nullptr};
#endif
};
