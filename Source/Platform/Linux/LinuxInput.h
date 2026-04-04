#pragma once

#include <Platform/Macros/IsPlatform.h>

#if IS_LINUX()

#include <cerrno>
#include <cstring>

#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <sys/ioctl.h>
#include <unistd.h>

#endif

class LinuxInput {
public:
    LinuxInput() noexcept
    {
#if IS_LINUX()
        initialize();
#endif
    }

    ~LinuxInput() noexcept
    {
#if IS_LINUX()
        shutdown();
#endif
    }

    LinuxInput(const LinuxInput&) = delete;
    LinuxInput(LinuxInput&&) = delete;
    LinuxInput& operator=(const LinuxInput&) = delete;
    LinuxInput& operator=(LinuxInput&&) = delete;

    void moveMouseRelative(float deltaX, float deltaY) noexcept
    {
#if IS_LINUX()
        if (!isAvailable())
            return;

        subPixelRemainderX += deltaX;
        subPixelRemainderY += deltaY;

        const auto integerDeltaX = roundToInt(subPixelRemainderX);
        const auto integerDeltaY = roundToInt(subPixelRemainderY);
        subPixelRemainderX -= static_cast<float>(integerDeltaX);
        subPixelRemainderY -= static_cast<float>(integerDeltaY);

        if (integerDeltaX == 0 && integerDeltaY == 0)
            return;

        emit(EV_REL, REL_X, integerDeltaX);
        emit(EV_REL, REL_Y, integerDeltaY);
        emit(EV_SYN, SYN_REPORT, 0);
#else
        static_cast<void>(deltaX);
        static_cast<void>(deltaY);
#endif
    }

    void simulateAttack() noexcept
    {
#if IS_LINUX()
        if (!isAvailable())
            return;

        emit(EV_KEY, BTN_LEFT, 1);
        emit(EV_SYN, SYN_REPORT, 0);
        emit(EV_KEY, BTN_LEFT, 0);
        emit(EV_SYN, SYN_REPORT, 0);
#endif
    }

    [[nodiscard]] bool isKeyboardKeyPressed(std::uint16_t code) const noexcept
    {
#if IS_LINUX()
        // Проверяем через SDL GetKeyboardState
        return keyboardStates[code];
#else
        static_cast<void>(code);
        return false;
#endif
    }

    [[nodiscard]] bool getMouseButtonState(std::uint8_t button) const noexcept
    {
#if IS_LINUX()
        return mouseButtonStates[button];
#else
        static_cast<void>(button);
        return false;
#endif
    }

    void updateKeyboardState(const bool* states) noexcept
    {
#if IS_LINUX()
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
#if IS_LINUX()
        if (button < 8)
            mouseButtonStates[button] = pressed;
#else
        static_cast<void>(button);
        static_cast<void>(pressed);
#endif
    }

    [[nodiscard]] bool isAvailable() const noexcept
    {
#if IS_LINUX()
        return fileDescriptor >= 0 && deviceCreated;
#else
        return false;
#endif
    }

private:
#if IS_LINUX()
    void initialize() noexcept
    {
        fileDescriptor = ::open("/dev/uinput", O_WRONLY | O_NONBLOCK);
        if (fileDescriptor < 0)
            return;

        if (!configureDevice()) {
            shutdown();
            return;
        }
        deviceCreated = true;
    }

    [[nodiscard]] bool configureDevice() const noexcept
    {
        if (::ioctl(fileDescriptor, UI_SET_EVBIT, EV_REL) < 0)
            return false;
        if (::ioctl(fileDescriptor, UI_SET_RELBIT, REL_X) < 0)
            return false;
        if (::ioctl(fileDescriptor, UI_SET_RELBIT, REL_Y) < 0)
            return false;

        if (::ioctl(fileDescriptor, UI_SET_EVBIT, EV_KEY) < 0)
            return false;
        if (::ioctl(fileDescriptor, UI_SET_KEYBIT, BTN_LEFT) < 0)
            return false;
        if (::ioctl(fileDescriptor, UI_SET_KEYBIT, BTN_RIGHT) < 0)
            return false;

        uinput_setup setup{};
        std::strncpy(setup.name, "Osiris Aimbot", UINPUT_MAX_NAME_SIZE - 1);
        setup.id.bustype = BUS_USB;
        setup.id.vendor = 0x6F73;
        setup.id.product = 0x6972;
        setup.id.version = 1;

        if (::ioctl(fileDescriptor, UI_DEV_SETUP, &setup) == 0)
            return ::ioctl(fileDescriptor, UI_DEV_CREATE) == 0;

        if (errno != EINVAL)
            return false;

        uinput_user_dev userDevice{};
        std::strncpy(userDevice.name, "Osiris Aimbot", UINPUT_MAX_NAME_SIZE - 1);
        userDevice.id.bustype = BUS_USB;
        userDevice.id.vendor = 0x6F73;
        userDevice.id.product = 0x6972;
        userDevice.id.version = 1;
        if (::write(fileDescriptor, &userDevice, sizeof(userDevice)) != sizeof(userDevice))
            return false;

        return ::ioctl(fileDescriptor, UI_DEV_CREATE) == 0;
    }

    void emit(unsigned short type, unsigned short code, int value) const noexcept
    {
        input_event event{};
        event.type = type;
        event.code = code;
        event.value = value;
        (void)::write(fileDescriptor, &event, sizeof(event));
    }

    void shutdown() noexcept
    {
        if (fileDescriptor >= 0) {
            if (deviceCreated)
                (void)::ioctl(fileDescriptor, UI_DEV_DESTROY);
            (void)::close(fileDescriptor);
        }
        fileDescriptor = -1;
        deviceCreated = false;
        subPixelRemainderX = 0.0f;
        subPixelRemainderY = 0.0f;
    }

    int fileDescriptor{-1};
    bool deviceCreated{false};
    float subPixelRemainderX{0.0f};
    float subPixelRemainderY{0.0f};
    bool keyboardStates[256]{};
    bool mouseButtonStates[8]{};

    [[nodiscard]] static int roundToInt(float value) noexcept
    {
        return static_cast<int>(value >= 0.0f ? value + 0.5f : value - 0.5f);
    }
#endif
};
