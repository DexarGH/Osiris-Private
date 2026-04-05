#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <random>

#include <Features/Aimbot/AimbotActivationKeys.h>
#include <GameClient/Entities/PlayerPawn.h>
#include <GameClient/EntitySystem/EntitySystem.h>
#include <GameClient/WorldToScreen/WorldToClipSpaceConverter.h>
#include <SDL/SdlDll.h>
#include <Utils/Lvalue.h>
#include <Utils/Logger.h>
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

        WorldToClipSpaceConverter<HookContext> worldToClipSpaceConverter{hookContext};
        
        // Флаг, что мы нашли врага на мушке
        bool enemyFound = false;

        hookContext.template make<EntitySystem>().forEachNetworkableEntityIdentity([&](const cs2::CEntityIdentity& entityIdentity) {
            const auto entityTypeInfo = hookContext.entityClassifier().classifyEntity(entityIdentity.entityClass);
            if (!entityTypeInfo.template is<cs2::C_CSPlayerPawn>())
                return;

            auto&& playerPawn = hookContext.template make<BaseEntity>(static_cast<cs2::C_BaseEntity*>(entityIdentity.entity)).template as<PlayerPawn>();
            if (!playerPawn)
                return;

            if (!playerPawn.isAlive().value_or(false))
                return;

            if (!playerPawn.isEnemy().value_or(false))
                return;

            if (playerPawn.isControlledByLocalPlayer())
                return;

            const auto targetOrigin = playerPawn.absOrigin();
            if (!targetOrigin.hasValue())
                return;

            // Проверяем 3 высоты (ноги, грудь, голова), чтобы попасть в хитбокс тела
            auto targetPos = targetOrigin.value();
            const float checkHeights[] = {25.0f, 45.0f, 65.0f};

            for (float heightOffset : checkHeights) {
                targetPos.z = targetOrigin.value().z + heightOffset;

                const auto clip = worldToClipSpaceConverter.toClipSpace(targetPos);
                if (!clip.onScreen() || clip.z < 0.0f)
                    continue;

                // NDC координаты (-1.0 до 1.0)
                const float inverseW = clip.w > 0.0001f ? 1.0f / clip.w : 0.0f;
                const float ndcX = clip.x * inverseW;
                const float ndcY = clip.y * inverseW;
                
                // Квадрат расстояния до центра экрана
                const float screenDist = ndcX * ndcX + ndcY * ndcY;

                // Порог: стреляем только если прицел точно на модели, а не рядом
                constexpr float triggerThreshold = 0.0015f;
                
                if (screenDist < triggerThreshold) {
                    enemyFound = true;
                    return; // Нашли врага — выходим из лямбды
                }
            }
        });

        if (!enemyFound)
            return;

        // Кулдаун (0.1 сек) для автоматической стрельбы
        const auto curtime = hookContext.globalVars().curtime();
        if (curtime.hasValue()) {
            if (curtime.value() - state().lastShotTime < 0.1f)
                return;
        }

        LOG("[TriggerBot] FIRING!");
        simulateAttackWithDelay();
        
        if (curtime.hasValue())
            state().lastShotTime = curtime.value();
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

        hookContext.input().simulateAttack();
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
