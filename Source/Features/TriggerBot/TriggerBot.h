#pragma once

#include <cstdint>
#include <random>

#include <Features/Aimbot/AimbotActivationKeys.h>
#include <GameClient/Entities/PlayerPawn.h>
#include <GameClient/EntitySystem/EntitySystem.h>
#include <SDL/SdlDll.h>
#include <Utils/Lvalue.h>
#include "TriggerBotConfigVariables.h"
#include "TriggerBotState.h"

template <typename HookContext>
class TriggerBot {
public:
    explicit TriggerBot(HookContext& hookContext) noexcept
        : hookContext{hookContext}
    {
    }

    void run() const
    {
        if (!GET_CONFIG_VAR(trigger_bot::Enabled))
            return;

        if (!isTriggerBotActive())
            return;

        auto&& localPawn = hookContext.activeLocalPlayerPawn();
        if (!localPawn)
            return;

        const auto crosshairEntityHandle = localPawn.getIdEntityIndex();
        if (!crosshairEntityHandle.has_value() || crosshairEntityHandle.value() == -1)
            return;

        auto&& baseEntity = hookContext.template make<EntitySystem>().getEntityFromHandle2(cs2::CEntityHandle{static_cast<std::uint32_t>(crosshairEntityHandle.value())});
        if (!baseEntity)
            return;

        auto&& playerPawn = baseEntity.template as<PlayerPawn>();
        if (!playerPawn)
            return;

        if (!playerPawn.isAlive().value_or(false))
            return;

        if (!playerPawn.isEnemy().value_or(false))
            return;

        if (playerPawn.isControlledByLocalPlayer())
            return;

        if (GET_CONFIG_VAR(trigger_bot::HeadOnly)) {
            // TODO: Requires bone checks — пока заглушка, всегда true
            static_cast<void>(true);
        }

        // Выполняем клик с рандомной задержкой
        simulateAttackWithDelay();
    }

    void onUnload() const noexcept
    {
    }

private:
    [[nodiscard]] bool isTriggerBotActive() const noexcept
    {
        const auto bindMode = GET_CONFIG_VAR(trigger_bot::BindModeType);
        if (bindMode == trigger_bot::BindMode::AlwaysOn)
            return true;

        const auto bindIndex = GET_CONFIG_VAR(trigger_bot::Bind);
        if (bindIndex >= aimbot::kActivationKeyBindings.size())
            return false;

        const auto& binding = aimbot::kActivationKeyBindings[bindIndex];

        switch (binding.inputType) {
        case aimbot::ActivationInputType::Mouse:
            return hookContext.input().getMouseButtonState(static_cast<std::uint8_t>(binding.code));
        case aimbot::ActivationInputType::Keyboard:
            return isKeyboardKeyPressed(binding.code);
        }

        return false;
    }

    void simulateAttackWithDelay() const
    {
        const auto minDelay = GET_CONFIG_VAR(trigger_bot::MinDelay);
        const auto maxDelay = GET_CONFIG_VAR(trigger_bot::MaxDelay);
        const auto delay = minDelay + (std::rand() % (maxDelay - minDelay + 1));

        // Мгновенный клик (задержка будет реализована через таймер)
        hookContext.input().simulateAttack();

        // TODO: Реализовать задержку через таймер
        static_cast<void>(delay);
    }

    [[nodiscard]] bool isKeyboardKeyPressed(std::size_t scancode) const noexcept
    {
        auto& getKeyboardState = state().getKeyboardState;
        if (!getKeyboardState)
            getKeyboardState = SdlDll{}.getKeyboardState();
        if (!getKeyboardState)
            return false;

        int numKeys = 0;
        const auto keyboardState = getKeyboardState(&numKeys);
        if (!keyboardState || scancode >= static_cast<std::size_t>(numKeys))
            return false;

        return keyboardState[scancode] != 0;
    }

    [[nodiscard]] auto& state() const noexcept
    {
        return hookContext.featuresStates().triggerBotState;
    }

    HookContext& hookContext;
};
